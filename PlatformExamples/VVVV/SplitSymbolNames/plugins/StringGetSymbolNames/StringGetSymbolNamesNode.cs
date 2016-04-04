#region usings
using System;
using System.IO;
using System.Linq;
using System.ComponentModel.Composition;
using System.Collections.Generic;
using System.Collections;

using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using VVVV.Utils.VColor;
using VVVV.Utils.VMath;

using VVVV.Core.Logging;
#endregion usings

namespace VVVV.Nodes
{
	#region PluginInfo
	[PluginInfo(Name = "GetSymbolNames", Category = "String", Help = "Basic template with one string in/out", Tags = "")]
	#endregion PluginInfo
	public class StringGetSymbolNamesNode : IPluginEvaluate
	{
		#region fields & pins
		[Input("Error String")]
		public IDiffSpread<string> FInput;

		[Input("Previous Frame")]
		public ISpread<string> FPrevious;
		
		[Input("Clear", IsBang=true)]
		public ISpread<bool> FInClear;
		
		[Input("Add", IsBang=true)]
		public ISpread<bool> FInAdd;
		
		[Input("Filename", StringType=StringType.Filename)]
		public IDiffSpread<string> FInFilename;
		
		[Input("Read file", IsBang=true)]
		public ISpread<bool> FInReadFile;
		
		[Output("Symbols")]
		public ISpread<string> FOutSymbols;

		[Output("Namespaces")]
		public ISpread<string> FOutNamespaces;

		[Output("DEF String")]
		public ISpread<string> FOutDEFString;
		
		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		public class NamespaceSort : IComparer<string>
		{
			CaseInsensitiveComparer caseiComp = new CaseInsensitiveComparer();
			
			public List<string> getNamespaces(string x)
			{
				var endOfNamespace = x.IndexOf("@@");
				if(endOfNamespace < 2) {
					return new List<string>();
				}
				
				var namespaces = new List<string>(x.Substring(0, endOfNamespace).Split('@'));
				
				namespaces.Reverse();
				return namespaces;
			}
			
			public string getNamespaceString(List<string> namespaces) {
				return string.Join("::", namespaces.ToArray());
			}
			
			public int Compare(List<string> xNamespaces, List<string> yNamespaces)
			{
				return caseiComp.Compare(getNamespaceString(xNamespaces), getNamespaceString(yNamespaces));
			}
			
			public int Compare(string x, string y)
			{	
				//first check if it has a namespace
				var xNamespaces = getNamespaces(x);
				var yNamespaces = getNamespaces(y);
				
				int namespaceCompare = Compare(xNamespaces, yNamespaces);
				if(namespaceCompare != 0) {
					return namespaceCompare;
				} else {
					return caseiComp.Compare(x, y);
				}
			}
		}
		
		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			var symbols = new List<string>();
			
			//take symbols from previous frame
			foreach(var symbol in FPrevious) {
				symbols.Add(symbol);
			}
			
			//take symbols from file
			if(FInReadFile[0]) {
				if (File.Exists(FInFilename[0])) {
					var lines = File.ReadAllLines(FInFilename[0]);
					for(int i=2; i<lines.Length; i++) {
						symbols.Add(lines[i]);
					}
				}
			}
			
			//take symbols from error string
			if(FInAdd[0]) {
				var lines = FInput[0].Split('\n');
				foreach(var line in lines) {
					if (line.Contains("LNK2001") || line.Contains("LNK2019")) {
						
						var lineStripped = line;
						
						var toIgnore = " referenced in function";
						var find = line.IndexOf(toIgnore);
						if(find != -1) {
							lineStripped = lineStripped.Substring(0, find);
						}
						
						var words = lineStripped.Split(' ');
						var lastWord = words[words.Length - 1];
						var symbolName = lastWord.Trim(new Char[] {'(', ')', '\n', '\r'});
						symbols.Add(symbolName);
					}
				}
			}
			
			//trim all whitespace
			{
				var trimmed = new List<string>();
				foreach(var symbol in symbols) {
					trimmed.Add(symbol.Trim());
				}
				symbols = trimmed;
			}
			
			//trim all empty
			symbols = symbols.Where(symbol => !String.IsNullOrEmpty(symbol)).ToList();
			
			//clear all symbols on request
			if(FInClear[0]) {
				symbols.Clear();
			}
			
			
			
			//build dictionary
			var namespaceSort = new NamespaceSort();
			symbols = symbols.Distinct().ToList();
			symbols.Sort(namespaceSort);
			
			
			//output symbols and namespaces
			FOutSymbols.SliceCount = 0;
			FOutNamespaces.SliceCount = 0;
			foreach(var symbol in symbols) {
				FOutSymbols.Add(symbol);
				FOutNamespaces.Add(namespaceSort.getNamespaceString(namespaceSort.getNamespaces(symbol)));
			}
			
			
			//output DEF string
			string defString;
			defString = "EXPORTS\n";
			foreach(var symbol in symbols) {
				defString += "\t" + symbol + "\n";
			}
			FOutDEFString[0] = defString;
		}
	}
}

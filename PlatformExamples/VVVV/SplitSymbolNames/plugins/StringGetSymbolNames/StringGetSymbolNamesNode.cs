#region usings
using System;
using System.ComponentModel.Composition;
using System.Collections.Generic;

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
		
		[Output("Output")]
		public ISpread<string> FOutput;

		[Import()]
		public ILogger FLogger;
		#endregion fields & pins

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			var lines = FInput[0].Split('\n');
			
			var symbols = new SortedSet<string>();
			foreach(var symbol in FPrevious) {
				symbols.Add(symbol);
			}
			
			if(FInAdd[0]) {
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
			
			//strip empty symbols
			symbols.RemoveWhere(symbol => String.IsNullOrEmpty(symbol));

			if(FInClear[0]) {
				symbols.Clear();
			}
				
			FOutput.SliceCount = 0;
			foreach(var symbol in symbols) {
				FOutput.Add(symbol);
			}
		}
	}
}

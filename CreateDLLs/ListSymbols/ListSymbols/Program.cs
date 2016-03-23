using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading.Tasks;
using System.Diagnostics;

namespace ListSymbols
{
	class Program
	{
		static void Main(string[] args)
		{
			if (args.Length == 0)
			{
				args = new string[] { "E:\\openFrameworks\\addons\\ofxRulr\\Core\\bin\\Debug\\Cored_x64.lib" };
			}

			bool passesTest = args.Length >= 2;
			for(int i=0; i<args.Length; i++)
			{
				if(i < args.Length -1)
				{
					passesTest &= Path.GetExtension(args[i]).ToUpper() == "LIB";
				} else
				{
					passesTest &= Path.GetExtension(args[i]).ToUpper() == "DEF";
				}
			}
			if(!passesTest)
			{
				Console.WriteLine("Usage : ");
				Console.WriteLine("    ListSymbols.exe library1.lib library2.lib exports.def");
				return;
			}

			
			foreach(var filename in args)
			{
				try
				{
					if (!File.Exists(filename))
					{
						throw (new Exception("File not found"));
					}

					var process = new Process
					{
						StartInfo = new ProcessStartInfo
						{
							FileName = "c:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\bin\\link.exe",
							Arguments = "/dump /all \"" + filename + "\"",
							UseShellExecute = false,
							RedirectStandardOutput = true
						}
					};
					process.Start();

					Console.WriteLine("Library " + Path.GetFileNameWithoutExtension(filename));
					Console.WriteLine("EXPORTS");

					bool reachedPublicSymbols = false;

					while(!process.StandardOutput.EndOfStream)
					{
						var line = process.StandardOutput.ReadLine();

						if(!reachedPublicSymbols)
						{
							if (line.Contains("public symbols"))
							{
								reachedPublicSymbols = true;
							}
							continue;
						}

						var columns = line.Split(new char[]{' '}, StringSplitOptions.RemoveEmptyEntries);
						if(columns.Length == 2)
						{
							var symbolName = columns[1];
							symbolName = "\t" + symbolName;
							Console.WriteLine(symbolName);
						}
					}

				}
				catch(Exception e)
				{
					Console.WriteLine(e.Message);
				}				
			}
		}
	}
}

using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Linq;
using System.Text;
using System.IO.Pipes;
using System.IO;
using System.Runtime.InteropServices;

namespace AblationClient
{
    /// <summary>
    /// It's really rough, but this tool is useful for controlling the output from the ablation console. Don't expect much from it ;)
    /// </summary>
    static class Program
    {
        const double TIMER_ELAPSED_OUTPUT = 5.0d;
        static bool showDelay = false;
        static bool noGui = false;
        static bool filter = false;
        static bool filtering = false;
        static string filterBeginTag = "###</ScriptHeader>###";

        public static System.IO.StreamWriter sw = null;
        public static string scriptFileName = "";

        static string[] ValidLines = new string[] // Identifies content that should be saved by AblationClientLite.exe. Content written to cout that doesn't begin with this tag is other content is saved as a comment '#'.
        {
            "createXRef(",
            "createXRefExternal(",
            "mark(",
            "#",
            "color = "
        };

        public static void Output(string line)
        {
            if (filter && !filtering)
            {
                if (line.Contains(filterBeginTag))
                {
                    filtering = true;
                }
            }

            if (filter && filtering && line.Length > 0)
            {
                bool prepend = true;
                foreach (string s in ValidLines)
                {
                    if (line.StartsWith(s))
                    {
                        prepend = false;
                        break;
                    }
                }
                if (prepend)
                {
                    Console.Write("#");
                    if (sw != null)
                        sw.Write("#");
                }
            }

            Console.WriteLine(line);
            if (sw != null)
                sw.WriteLine(line);

        }

        static void Run()
        {
            DateTime last = DateTime.Now;

            string line;
            string previousLine = "";

            TimeSpan elapsed;
            bool outputTime = false;

            while ((line = Console.ReadLine()) != null)
            {
                try
                {
                    if (showDelay)
                    {
                        elapsed = (DateTime.Now - last);
                        if (!outputTime && elapsed.TotalSeconds >= TIMER_ELAPSED_OUTPUT)
                            outputTime = true;

                        if (outputTime)
                        {
                            Output("#              ----- " + DateTime.Now.ToShortDateString() + " @ " + DateTime.Now.ToShortTimeString() + "\t" + elapsed.TotalSeconds.ToString("{0.000}") + " seconds elapsed --------------------------------");
                            outputTime = false;
                        }
                        last = DateTime.Now;
                    }
                    if (line.CompareTo(previousLine) != 0)
                        Output(line);
                    previousLine = line;
                }
                catch (IOException ex)
                {
                    Console.Error.WriteLine(ex.ToString());
                    return;
                }

            }

            if (sw != null)
            {
                sw.Flush();
                sw.Close();
            }
        }


        static void Usage()
        {
            Console.WriteLine("It's really rough, but this tool is useful for controlling the output from the ablation console. Don't expect much from it ;)\n");

            Console.WriteLine("Usage:");
            Console.WriteLine("\t" + System.AppDomain.CurrentDomain.FriendlyName + " [output filename (Optional)]");
            
            Console.WriteLine("\t\t[-a         \t(optional)\tAppend to existing log file, default = create new]");
            Console.WriteLine("\t\t[-append    \t(optional)\tAppend to existing log file, default = create new]");
            Console.WriteLine("\t\t[-show_delay\t(optional)\tDisplay elapsed time between messages more than 5 seconds apart]");
            Console.WriteLine("\t\t[-no_gui    \t(optional)\tDo not display the GUI interface used to change colors, etc.]");
            Console.WriteLine("\t\t[-filter    \t(optional)\tFilter the output of unexpected script content]\n");


            for (int i = 0; i < 3; i++)
            {
                Console.Write(".");
                System.Threading.Thread.Sleep(333);
            }
            Console.WriteLine();
        }
        
        static bool ParseArgs(string[] args)
        {
            List<string> cmdLineArgs = new List<string>(args);

            showDelay = cmdLineArgs.Contains<string>("-show_delay");
            if (showDelay)
                cmdLineArgs.Remove("-show_delay");

            noGui = cmdLineArgs.Contains<string>("-no_gui");
            if (showDelay)
                cmdLineArgs.Remove("-no_gui");


            bool append = cmdLineArgs.Contains<string>("-a");
            if (append)
            {
                cmdLineArgs.Remove("-a");
            }
            else
            {
                append = cmdLineArgs.Contains<string>("-append");
                if (append)
                    cmdLineArgs.Remove("-append");
            }

            filter = !cmdLineArgs.Contains<string>("-no_filter");
            if (!filter)
                cmdLineArgs.Remove("-no_filter");

            if (cmdLineArgs.Count > 0)
            {
                foreach (string s in cmdLineArgs)
                {
                    if (!s.StartsWith("-"))
                    {
                        try
                        {
                            sw = new StreamWriter(s, append);
                            scriptFileName = s;
                            sw.AutoFlush = true;
                            Console.WriteLine((append ? "Appending" : "Logging") + " to file: " + args[0]);
                        }
                        catch (Exception ex)
                        {
                            noGui = true;
                            Console.WriteLine("Error opening requested script file: " + s + " ...\n\n" + ex.ToString());
                        }

                        return true;
                    }

                }

            }

            return true;
        }


        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            if (args == null || args.Length == 0)
                Usage();
            if (args.Length >= 1)
            {
                if (!ParseArgs(args))
                {
                    Usage();
                    return;
                }
            }

            if (!noGui)
            {
                System.Threading.Thread thread = new System.Threading.Thread(new System.Threading.ThreadStart(Run));
                thread.Start();

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new AblationClientForm());
            }
            else
            {
                Run();
                
            }
        }
    }
}

/* Copyright 2010-2011 Ilkka Halila
This file is part of Goblin Camp.

Goblin Camp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Goblin Camp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Goblin Camp. If not, see <http://www.gnu.org/licenses/>.*/

using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Threading;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;
using System.Management;

using ICSharpCode.SharpZipLib.Zip;
using ICSharpCode.SharpZipLib.Core;

//
// Warning: poor quality C# ahead.
//

namespace crash
{
    static class Program
    {
        static CrashForm form;
        static string dataDir;
        static string zipFile;
        static string dumpFile;
        static ManagementScope wmiScope;

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            if (args.Length == 0)
            {
                MessageBox.Show("Invalid argument count.", "goblin-camp-crash", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            dumpFile = args[0];

            wmiScope = new ManagementScope();

            form = new CrashForm();
            GetGCDataDir();
            Thread zipThread = new Thread(CreateArchive);
            zipThread.Start();
            Application.Run(form);
        }

        [DllImport("shell32.dll")]
        public static extern Int32 SHGetFolderPath(
            IntPtr hwndOwner, Int32 nFolder, IntPtr hToken,
            UInt32 dwFlags, StringBuilder pszPath
        );
        
        static void GetGCDataDir()
        {
            StringBuilder path = new System.Text.StringBuilder(4096);

            const Int32 CSIDL_PERSONAL      = 0x0005;
            const UInt32 SHGFP_TYPE_CURRENT = 0x0000;
            SHGetFolderPath(IntPtr.Zero, CSIDL_PERSONAL, IntPtr.Zero, SHGFP_TYPE_CURRENT, path);

            path.Append("\\My Games\\Goblin Camp\\");
            dataDir = path.ToString();

            path.Append("crashes\\");

            Directory.SetCurrentDirectory(path.ToString());

            path.Append(Path.GetFileNameWithoutExtension(dumpFile));
            path.Append(".zip");

            zipFile = path.ToString();
            form.SetZipFilename(zipFile);
        }

        static void CreateZipEntry(ZipOutputStream zip, string filename)
        {
            ZipEntry entry = new ZipEntry(filename);
            entry.DateTime = DateTime.UtcNow;
            zip.PutNextEntry(entry);
        }

        static void AddToZip(ZipOutputStream zip, string filename, string source)
        {
            CreateZipEntry(zip, filename);

            using (FileStream fs = File.OpenRead(source))
            {
                byte[] buffer = new byte[8192];
                int read;
                do
                {
                    read = fs.Read(buffer, 0, buffer.Length);
                    zip.Write(buffer, 0, read);
                } while (read > 0);
            }
        }

        static void AddToZip(ZipOutputStream zip, string filename)
        {
            AddToZip(zip, filename, filename);
        }

        static void AddToZip(ZipOutputStream zip, string filename, byte[] data)
        {
            AddToZip(zip, filename, data, data.Length);
        }

        static void AddToZip(ZipOutputStream zip, string filename, byte[] data, int length)
        {
            CreateZipEntry(zip, filename);

            zip.Write(data, 0, length);
        }

        static ManagementObjectCollection QueryWMI(string query)
        {
            WqlObjectQuery wmiQuery = new WqlObjectQuery(query);
            ManagementObjectSearcher wmiSearch = new ManagementObjectSearcher(wmiScope, wmiQuery);

            return wmiSearch.Get();
        }

        static string GetWMIProperty(ManagementObject obj, string property)
        {
            if (obj == null)
            {
                return "<null>";
            }

            try
            {
                return obj.GetPropertyValue(property).ToString();
            }
            catch (Exception e)
            {
                return "[Exception occured: " + e.ToString() + "]";
            }
        }

        static UInt64 GetWMIIntProperty(ManagementObject obj, string property)
        {
            try
            {
                return UInt64.Parse(GetWMIProperty(obj, property));
            }
            catch (Exception)
            {
                return 0;
            }
        }

        delegate void UIUpdateDelegate();
        static void CreateArchive()
        {
            try
            {
                using (ZipOutputStream zip = new ZipOutputStream(File.Create(zipFile)))
                {
                    zip.SetComment("Created by Goblin Camp crash reporter service.");
                    zip.SetLevel(6);

                    AddToZip(zip, dumpFile);
                    AddToZip(zip, "config.py", "..\\config.py");
                    AddToZip(zip, "terminal.png", "..\\terminal.png");
                    AddToZip(zip, "goblin-camp.log", "..\\goblin-camp.log");

                    DirectoryInfo modsDir = new DirectoryInfo(dataDir + "mods");
                    StringBuilder modsList = new StringBuilder(1024);
                    modsList.Append("=========[ Installed mods ]=========\n");
                    foreach (DirectoryInfo subdir in modsDir.GetDirectories())
                    {
                        modsList.AppendFormat(" * {0}\n", subdir.Name);

                        if (File.Exists(subdir.FullName + "\\mod.dat"))
                        {
                            AddToZip(zip, "mods\\" + subdir.Name + ".dat", subdir.FullName + "\\mod.dat");
                        }
                    }

                    UTF8Encoding utf8 = new UTF8Encoding();
                    AddToZip(zip, "mods.txt", utf8.GetBytes(modsList.ToString()));

                    StringBuilder wmiCollectedData = new StringBuilder(8192);

                    // We are querying local WMI to get following data:
                    //  - video cards, along with drivers' info
                    wmiCollectedData.Append("=========[ Installed video controllers ]=========\n");

                    foreach (ManagementObject obj in QueryWMI("SELECT * FROM Win32_VideoController"))
                    {
                        wmiCollectedData.AppendFormat(" * {0} [{1}]\n", GetWMIProperty(obj, "Description"), GetWMIProperty(obj, "VideoProcessor"));
                        wmiCollectedData.AppendFormat("   VRAM: {0}MB\n", GetWMIIntProperty(obj, "AdapterRAM") / (1024 * 1024));
                        wmiCollectedData.AppendFormat("   BPP: {0}\n", GetWMIProperty(obj, "CurrentBitsPerPixel"));
                        wmiCollectedData.AppendFormat(
                            "   Driver: {0} ({1})\n",
                            GetWMIProperty(obj, "DriverVersion"), GetWMIProperty(obj, "DriverDate")
                        );
                        foreach (string driver in GetWMIProperty(obj, "InstalledDisplayDrivers").ToString().Split(','))
                        {
                            wmiCollectedData.AppendFormat("      {0}\n", driver);
                        }
                        wmiCollectedData.AppendFormat("   Current video mode: {0}\n", GetWMIProperty(obj, "VideoModeDescription"));
                    }

                    //  - attached monitors
                    wmiCollectedData.Append("\n=========[ Attached monitors ]=========\n");

                    foreach (ManagementObject obj in QueryWMI("SELECT * FROM Win32_DesktopMonitor"))
                    {
                        wmiCollectedData.AppendFormat(" * {0}\n", GetWMIProperty(obj, "Description"));
                        wmiCollectedData.AppendFormat(
                            "   Resolution: {0} x {1}\n",
                            GetWMIProperty(obj, "ScreenWidth"), GetWMIProperty(obj, "ScreenHeight")
                        );
                    }

                    //  - operating system info
                    wmiCollectedData.Append("\n=========[ Operating system ]=========\n");
                    
                    foreach (ManagementObject obj in QueryWMI("SELECT * FROM Win32_OperatingSystem WHERE Primary = TRUE"))
                    {
                        wmiCollectedData.AppendFormat("{0}\n", GetWMIProperty(obj, "Caption"));
                        wmiCollectedData.AppendFormat(
                            "Physical memory (free/total): {0}MB / {1}MB\n",
                            GetWMIIntProperty(obj, "FreePhysicalMemory") / 1024,
                            GetWMIIntProperty(obj, "TotalVisibleMemorySize") / 1024
                        );
                        wmiCollectedData.AppendFormat("Architecture: {0}\n", GetWMIProperty(obj, "OSArchitecture"));

                        break; // I'm lazy
                    }

                    AddToZip(zip, "wmi.txt", utf8.GetBytes(wmiCollectedData.ToString()));
                }
            }
            catch (Exception e)
            {
                MessageBox.Show(
                    e.ToString(), "Error while creating the archive.",
                    MessageBoxButtons.OK, MessageBoxIcon.Error
                );
            }

            form.Invoke(new UIUpdateDelegate(form.ArchiveReady));
        }
    }
}

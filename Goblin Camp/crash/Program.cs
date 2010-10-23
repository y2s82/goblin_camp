/* Copyright 2010 Ilkka Halila
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

using ICSharpCode.SharpZipLib.Zip;
using ICSharpCode.SharpZipLib.Core;

namespace crash
{
    static class Program
    {
        static CrashForm form;
        static string dataDir;
        static string zipFile;
        static string dumpFile;

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
                    AddToZip(zip, "config.ini", "..\\config.ini");
                    AddToZip(zip, "goblin-camp.log", "..\\goblin-camp.log");
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

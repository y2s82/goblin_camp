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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;

namespace crash
{
    public partial class CrashForm : Form
    {
        public CrashForm()
        {
            InitializeComponent();
        }

        public void SetZipFilename(string filename)
        {
            zipBox.Text = filename;
        }

        public void ArchiveReady()
        {
            progressLabel.Visible = waitBar.Visible = false;
            okButton.Enabled = true;
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void forumsLink_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            try
            {
                Process.Start("http://www.goblincamp.com/forum");
            }
            catch (System.ComponentModel.Win32Exception exc)
            {
                if (exc.ErrorCode == -2147467259)
                {
                    MessageBox.Show(exc.Message);
                }
            }
        }
    }
}

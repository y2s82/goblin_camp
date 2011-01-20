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

namespace crash
{
    partial class CrashForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CrashForm));
            this.titleLabel = new System.Windows.Forms.Label();
            this.explainLabel = new System.Windows.Forms.Label();
            this.waitBar = new System.Windows.Forms.ProgressBar();
            this.okButton = new System.Windows.Forms.Button();
            this.zipBox = new System.Windows.Forms.TextBox();
            this.zipLabel = new System.Windows.Forms.Label();
            this.progressLabel = new System.Windows.Forms.Label();
            this.forumsLink = new System.Windows.Forms.LinkLabel();
            this.SuspendLayout();
            // 
            // titleLabel
            // 
            this.titleLabel.AutoSize = true;
            this.titleLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.titleLabel.Location = new System.Drawing.Point(12, 9);
            this.titleLabel.Name = "titleLabel";
            this.titleLabel.Size = new System.Drawing.Size(242, 20);
            this.titleLabel.TabIndex = 5;
            this.titleLabel.Text = "Oops. Goblin Camp has crashed.";
            // 
            // explainLabel
            // 
            this.explainLabel.AutoSize = true;
            this.explainLabel.Location = new System.Drawing.Point(13, 40);
            this.explainLabel.Name = "explainLabel";
            this.explainLabel.Size = new System.Drawing.Size(594, 182);
            this.explainLabel.TabIndex = 2;
            this.explainLabel.Text = resources.GetString("explainLabel.Text");
            // 
            // waitBar
            // 
            this.waitBar.Location = new System.Drawing.Point(16, 283);
            this.waitBar.Name = "waitBar";
            this.waitBar.Size = new System.Drawing.Size(220, 25);
            this.waitBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this.waitBar.TabIndex = 3;
            // 
            // okButton
            // 
            this.okButton.Enabled = false;
            this.okButton.Location = new System.Drawing.Point(487, 255);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(120, 25);
            this.okButton.TabIndex = 0;
            this.okButton.Text = "OK";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // zipBox
            // 
            this.zipBox.Location = new System.Drawing.Point(137, 229);
            this.zipBox.Name = "zipBox";
            this.zipBox.ReadOnly = true;
            this.zipBox.Size = new System.Drawing.Size(470, 20);
            this.zipBox.TabIndex = 1;
            // 
            // zipLabel
            // 
            this.zipLabel.AutoSize = true;
            this.zipLabel.Location = new System.Drawing.Point(13, 232);
            this.zipLabel.Name = "zipLabel";
            this.zipLabel.Size = new System.Drawing.Size(116, 13);
            this.zipLabel.TabIndex = 5;
            this.zipLabel.Text = "You will find the ZIP at:";
            // 
            // progressLabel
            // 
            this.progressLabel.AutoSize = true;
            this.progressLabel.Location = new System.Drawing.Point(60, 267);
            this.progressLabel.Name = "progressLabel";
            this.progressLabel.Size = new System.Drawing.Size(126, 13);
            this.progressLabel.TabIndex = 6;
            this.progressLabel.Text = "... Creating the archive ...";
            // 
            // forumsLink
            // 
            this.forumsLink.AutoSize = true;
            this.forumsLink.Location = new System.Drawing.Point(511, 295);
            this.forumsLink.Name = "forumsLink";
            this.forumsLink.Size = new System.Drawing.Size(101, 13);
            this.forumsLink.TabIndex = 7;
            this.forumsLink.TabStop = true;
            this.forumsLink.Text = "Goblin Camp forums";
            this.forumsLink.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.forumsLink_LinkClicked);
            // 
            // CrashForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.ClientSize = new System.Drawing.Size(624, 317);
            this.Controls.Add(this.forumsLink);
            this.Controls.Add(this.progressLabel);
            this.Controls.Add(this.zipLabel);
            this.Controls.Add(this.zipBox);
            this.Controls.Add(this.okButton);
            this.Controls.Add(this.waitBar);
            this.Controls.Add(this.explainLabel);
            this.Controls.Add(this.titleLabel);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "CrashForm";
            this.Text = "Goblin Camp crash reporter service";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label titleLabel;
        private System.Windows.Forms.Label explainLabel;
        private System.Windows.Forms.ProgressBar waitBar;
        private System.Windows.Forms.Button okButton;
        private System.Windows.Forms.TextBox zipBox;
        private System.Windows.Forms.Label zipLabel;
        private System.Windows.Forms.Label progressLabel;
        private System.Windows.Forms.LinkLabel forumsLink;
    }
}


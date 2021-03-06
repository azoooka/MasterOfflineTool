﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Questionnaire
{
    public partial class Form1 : Form
    {

        public Form1()
        {
            InitializeComponent();
        }
        private String GetTimestamp(DateTime value)
        {
            return value.ToString("yyyy-MM-dd-HH-mm-ss-fff");
        }
        private void Submit_Click(object sender, EventArgs e)
        {
            string contestantName = GetTimestamp(DateTime.Now);
            string newDirectory = "DataLogs/" + "DataLogs" + contestantName;
            Directory.CreateDirectory(newDirectory);

            StreamWriter file = new StreamWriter(newDirectory + "/" + "QuestionnaireAnsweres.txt");
            foreach (var group in this.Controls.OfType<GroupBox>())
            {
                RadioButton theButton = group.Controls.OfType<RadioButton>().FirstOrDefault(n => n.Checked);
                if (theButton == null)
                {
                    MessageBox.Show("Please answer all the questions");
                    // Close and delete file and folder
                    file.Close();
                    File.Delete(newDirectory + "/" + "QuestionnaireAnsweres.txt");
                    Directory.Delete(newDirectory);
                    return;
                }
                string ans = group.Text + ": " + group.Controls.OfType<RadioButton>().FirstOrDefault(n => n.Checked).Text;
                file.WriteLine(ans);
            }
            file.Close();

            string[] allFiles = Directory.GetFiles("../", "*.rawData");
            for (int i = 0; i < allFiles.Length; i++)
            {
                string fileName = allFiles[i].Substring(allFiles[i].LastIndexOf("/") + 1);
                File.Move(allFiles[i], newDirectory + "/" + fileName);
            }
            Application.Exit();
        }

        private void radioButton4_CheckedChanged(object sender, EventArgs e)
        {

        }
    }
}

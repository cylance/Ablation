using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AblationClient
{
    public partial class AblationClientForm : Form
    {
        bool colorInitialized = false;

        public AblationClientForm()
        {
            InitializeComponent();

            colorDialog1.AnyColor = true;
            colorDialog1.CustomColors = new int[] { 0x7BF0D3, 0xFFFFFF, 0xE8E8E8, 0xCCCCCC, 0x88F8D8, 0x5BD7AE, 0xEDD5BE, 0xE3C09D, 0xFFD0DC, 0xE3AAAA, 0xFFFFC0, 0xFFFF80, 0xB3B3F6, 0x7777E0, 0x6EC5FF, 0x40B0FF };
            colorDialog1.FullOpen = true;
        }


        private void setColorButton_Click(object sender, EventArgs e)
        {
            if(!colorInitialized)
            {
                if (!PickColor())
                    return;
            }

            Color c = setColorButton.BackColor;
            currentColorButton.BackColor = c;
            Program.Output("color = 0x" + c.B.ToString("X2") + c.G.ToString("X2") + c.G.ToString("X2"));
        }

        bool PickColor()
        {
            colorDialog1.ShowDialog();

            Color c = colorDialog1.Color;
            if (c.B == 0 && c.G == 0 && c.G == 0)
                return false;

            colorInitialized = true;

            setColorButton.BackColor = colorDialog1.Color;

            return true;
        }


        bool ValidScriptFile()
        {
            if (String.IsNullOrEmpty(Program.scriptFileName) || !System.IO.File.Exists(Program.scriptFileName))
                return false;
            return true;
        }

        private void openColorSelectorButton_Click(object sender, EventArgs e)
        {
            PickColor();
        }

        private void copyCurrentScriptButton_Click(object sender, EventArgs e)
        {
            if (!ValidScriptFile())
                return;

            string destFile = System.IO.Path.GetFileNameWithoutExtension(Program.scriptFileName) + " - Copy" + System.IO.Path.GetExtension(Program.scriptFileName);

            int i = 2;
            while(System.IO.File.Exists(destFile))
            {
                destFile = System.IO.Path.GetFileNameWithoutExtension(Program.scriptFileName) + " - Copy (" + i.ToString() + ")" + System.IO.Path.GetExtension(Program.scriptFileName);

                if(i > 999)
                    return;
                i++;
            }

            try
            {
                System.IO.File.Copy(Program.scriptFileName, destFile);
                Console.WriteLine("Copied " + Program.scriptFileName + " to " + destFile);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }

        private void copyCurrentScriptAsButton_Click(object sender, EventArgs e)
        {
            if (!ValidScriptFile())
                return;

            // Displays a SaveFileDialog
            SaveFileDialog saveFileDialog1 = new SaveFileDialog();
            saveFileDialog1.Filter = "Python Script|*.py|All Files|*.*";
            saveFileDialog1.Title = "Save " + Program.scriptFileName + " as File";
            saveFileDialog1.ShowDialog();
            
            if(!String.IsNullOrEmpty(saveFileDialog1.FileName))
            {
                try
                {
                    System.IO.File.Copy(Program.scriptFileName, saveFileDialog1.FileName, true);
                    Console.WriteLine("Copied " + Program.scriptFileName + " to " + System.IO.Path.GetFileName(saveFileDialog1.FileName));
                }
                catch(Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                }
            }
            
        }






        
    }
}

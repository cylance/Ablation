namespace AblationClient
{
    partial class AblationClientForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AblationClientForm));
            this.colorDialog1 = new System.Windows.Forms.ColorDialog();
            this.setColorButton = new System.Windows.Forms.Button();
            this.copyCurrentScriptButton = new System.Windows.Forms.Button();
            this.copyCurrentScriptAsButton = new System.Windows.Forms.Button();
            this.openColorSelectorButton = new System.Windows.Forms.Button();
            this.currentColorButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // setColorButton
            // 
            this.setColorButton.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.setColorButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.setColorButton.Location = new System.Drawing.Point(12, 12);
            this.setColorButton.Name = "setColorButton";
            this.setColorButton.Size = new System.Drawing.Size(360, 57);
            this.setColorButton.TabIndex = 0;
            this.setColorButton.Text = "Set Color";
            this.setColorButton.UseVisualStyleBackColor = false;
            this.setColorButton.Click += new System.EventHandler(this.setColorButton_Click);
            // 
            // copyCurrentScriptButton
            // 
            this.copyCurrentScriptButton.Location = new System.Drawing.Point(12, 201);
            this.copyCurrentScriptButton.Name = "copyCurrentScriptButton";
            this.copyCurrentScriptButton.Size = new System.Drawing.Size(360, 57);
            this.copyCurrentScriptButton.TabIndex = 3;
            this.copyCurrentScriptButton.Text = "Copy Current Script";
            this.copyCurrentScriptButton.UseVisualStyleBackColor = true;
            this.copyCurrentScriptButton.Click += new System.EventHandler(this.copyCurrentScriptButton_Click);
            // 
            // copyCurrentScriptAsButton
            // 
            this.copyCurrentScriptAsButton.Location = new System.Drawing.Point(12, 264);
            this.copyCurrentScriptAsButton.Name = "copyCurrentScriptAsButton";
            this.copyCurrentScriptAsButton.Size = new System.Drawing.Size(360, 57);
            this.copyCurrentScriptAsButton.TabIndex = 4;
            this.copyCurrentScriptAsButton.Text = "Copy Current Script As";
            this.copyCurrentScriptAsButton.UseVisualStyleBackColor = true;
            this.copyCurrentScriptAsButton.Click += new System.EventHandler(this.copyCurrentScriptAsButton_Click);
            // 
            // openColorSelectorButton
            // 
            this.openColorSelectorButton.Location = new System.Drawing.Point(12, 138);
            this.openColorSelectorButton.Name = "openColorSelectorButton";
            this.openColorSelectorButton.Size = new System.Drawing.Size(360, 57);
            this.openColorSelectorButton.TabIndex = 5;
            this.openColorSelectorButton.Text = "Select Color";
            this.openColorSelectorButton.UseVisualStyleBackColor = true;
            this.openColorSelectorButton.Click += new System.EventHandler(this.openColorSelectorButton_Click);
            // 
            // currentColorButton
            // 
            this.currentColorButton.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.currentColorButton.Enabled = false;
            this.currentColorButton.Location = new System.Drawing.Point(12, 75);
            this.currentColorButton.Name = "currentColorButton";
            this.currentColorButton.Size = new System.Drawing.Size(360, 57);
            this.currentColorButton.TabIndex = 6;
            this.currentColorButton.Text = "Current Color";
            this.currentColorButton.UseVisualStyleBackColor = false;
            // 
            // AblationClientForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(384, 336);
            this.Controls.Add(this.currentColorButton);
            this.Controls.Add(this.openColorSelectorButton);
            this.Controls.Add(this.copyCurrentScriptAsButton);
            this.Controls.Add(this.copyCurrentScriptButton);
            this.Controls.Add(this.setColorButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "AblationClientForm";
            this.Text = "AblationClient";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ColorDialog colorDialog1;
        private System.Windows.Forms.Button setColorButton;
        private System.Windows.Forms.Button copyCurrentScriptButton;
        private System.Windows.Forms.Button copyCurrentScriptAsButton;
        private System.Windows.Forms.Button openColorSelectorButton;
        private System.Windows.Forms.Button currentColorButton;
    }
}


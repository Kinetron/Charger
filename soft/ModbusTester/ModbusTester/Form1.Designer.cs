namespace ModbusTester1
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
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
		///  Required method for Designer support - do not modify
		///  the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			components = new System.ComponentModel.Container();
			buttonTest = new Button();
			bindingSource1 = new BindingSource(components);
			buttonRun = new Button();
			imageList1 = new ImageList(components);
			labelResult = new Label();
			buttonRunСycle = new Button();
			timerReadData = new System.Windows.Forms.Timer(components);
			((System.ComponentModel.ISupportInitialize)bindingSource1).BeginInit();
			SuspendLayout();
			// 
			// buttonTest
			// 
			buttonTest.Location = new Point(434, 648);
			buttonTest.Margin = new Padding(5, 6, 5, 6);
			buttonTest.Name = "buttonTest";
			buttonTest.Size = new Size(365, 56);
			buttonTest.TabIndex = 0;
			buttonTest.Text = "Тест";
			buttonTest.UseVisualStyleBackColor = true;
			buttonTest.Click += buttonTest_Click;
			// 
			// bindingSource1
			// 
			bindingSource1.CurrentChanged += bindingSource1_CurrentChanged;
			// 
			// buttonRun
			// 
			buttonRun.Location = new Point(480, 272);
			buttonRun.Margin = new Padding(5, 6, 5, 6);
			buttonRun.Name = "buttonRun";
			buttonRun.Size = new Size(129, 46);
			buttonRun.TabIndex = 1;
			buttonRun.Text = "Пуск";
			buttonRun.UseVisualStyleBackColor = true;
			buttonRun.Click += buttonRun_Click;
			// 
			// imageList1
			// 
			imageList1.ColorDepth = ColorDepth.Depth8Bit;
			imageList1.ImageSize = new Size(16, 16);
			imageList1.TransparentColor = Color.Transparent;
			// 
			// labelResult
			// 
			labelResult.AutoSize = true;
			labelResult.Location = new Point(393, 458);
			labelResult.Margin = new Padding(5, 0, 5, 0);
			labelResult.Name = "labelResult";
			labelResult.Size = new Size(68, 30);
			labelResult.TabIndex = 2;
			labelResult.Text = "label1";
			// 
			// buttonRunСycle
			// 
			buttonRunСycle.Location = new Point(836, 115);
			buttonRunСycle.Name = "buttonRunСycle";
			buttonRunСycle.Size = new Size(131, 40);
			buttonRunСycle.TabIndex = 3;
			buttonRunСycle.Text = "Цикл";
			buttonRunСycle.UseVisualStyleBackColor = true;
			buttonRunСycle.Click += buttonRunСycle_Click;
			// 
			// timerReadData
			// 
			timerReadData.Tick += timerReadData_Tick;
			// 
			// Form1
			// 
			AutoScaleDimensions = new SizeF(12F, 30F);
			AutoScaleMode = AutoScaleMode.Font;
			ClientSize = new Size(1371, 900);
			Controls.Add(buttonRunСycle);
			Controls.Add(labelResult);
			Controls.Add(buttonRun);
			Controls.Add(buttonTest);
			Margin = new Padding(5, 6, 5, 6);
			Name = "Form1";
			Text = "Form1";
			FormClosed += Form1_FormClosed;
			Load += Form1_Load;
			((System.ComponentModel.ISupportInitialize)bindingSource1).EndInit();
			ResumeLayout(false);
			PerformLayout();
		}

		#endregion

		private Button buttonTest;
        private BindingSource bindingSource1;
        private Button buttonRun;
        private ImageList imageList1;
        private Label labelResult;
		private Button buttonRunСycle;
		private System.Windows.Forms.Timer timerReadData;
	}
}
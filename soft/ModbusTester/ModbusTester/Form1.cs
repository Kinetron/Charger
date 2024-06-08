using System.IO.Ports;
using Modbus.Device;
using Modbus.Serial;

namespace ModbusTester1
{
	public partial class Form1 : Form
	{
		private const int SerialTimeout = 1000; //1second
		private const int ReadInterval = 100;

		private bool _serialIsOpen = false;
		private int _readCounter = 1;


		private SerialPort port = new SerialPort("COM5");
		SerialPortAdapter adapter;
		// create modbus master
		IModbusSerialMaster master;
		public Form1()
		{
			InitializeComponent();
			timerReadData.Interval = ReadInterval;
		}

		private void buttonTest_Click(object sender, EventArgs e)
		{
			OpenPort();

			// master.WriteSingleCoil(0xAA, 0, true);
			// Task.Delay(1000);
			ushort[] registers1 = master.ReadInputRegisters(171, 0, 2);
			// write three registers
			// master.WriteMultipleRegisters(slaveId, startAddress, registers);

			byte[] asByte = new byte[4];
			Buffer.BlockCopy(registers1, 0, asByte, 0, 4);
			float result = BitConverter.ToSingle(asByte, 0);

			labelResult.Text = result.ToString();

		}

		private void bindingSource1_CurrentChanged(object sender, EventArgs e)
		{

		}

		private void Form1_Load(object sender, EventArgs e)
		{
		}

		/// <summary>
		/// Open port and config modbus.
		/// </summary>
		private void OpenPort()
		{
			if (_serialIsOpen) return;

			//Configure serial port
			port.BaudRate = 9600;
			port.DataBits = 8;
			port.Parity = Parity.None;//Parity.Even;
			port.StopBits = StopBits.One;
			port.Open();

			adapter = new SerialPortAdapter(port);
			//Create modbus master
			master = ModbusSerialMaster.CreateRtu(adapter);
			master.Transport.ReadTimeout = master.Transport.WriteTimeout = SerialTimeout;
			_serialIsOpen = true;
		}

		private void Form1_FormClosed(object sender, FormClosedEventArgs e)
		{
			timerReadData.Stop();
			port.Close();
		}

		private void buttonRun_Click(object sender, EventArgs e)
		{
			byte slaveId = 1;
			ushort startAddress = 170;
			ushort[] registers = new ushort[] { 0xFF };


			master.WriteSingleCoil(170, 0, true);
		}

		private void buttonRunÑycle_Click(object sender, EventArgs e)
		{
			OpenPort();
			timerReadData.Start();
		}

		private void timerReadData_Tick(object sender, EventArgs e)
		{
			ushort[] registers1 = master.ReadInputRegisters(171, 0, 2);
			byte[] asByte = new byte[4];
			Buffer.BlockCopy(registers1, 0, asByte, 0, 4);
			float result = BitConverter.ToSingle(asByte, 0);

			this.Invoke(() =>
			{
				labelResult.Text = result.ToString();
			});

			using (StreamWriter w = File.AppendText("test.txt"))
			{
				w.WriteLine($"{_readCounter} {result.ToString()}");
			}

			_readCounter++;
		}
	}
}
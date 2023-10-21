#include <iostream>
#include <string.h>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <math.h>
#include <stdio.h>
#include <windows.h>
#include <chrono>
#include <thread>
#include <signal.h>
#include<Definitions.h>





using namespace std;

typedef void* HANDLE;
typedef int BOOL;

HANDLE KeyHandle = 0;
unsigned short NodeId = 1;
int baudrate = 0;
int encoderCountsPerRevolution = 4096;
float logInterval = 1.0;

string destinationName;
string deviceName;
string protocolStackName;
string interfaceName;
string portName;
string opt;
const string programName = "drive_motor";
const string fileName = "motor_data";
const string directoryName = "motor_data";
DWORD pErrorCode;

ofstream outputFile;


// 使用されている関数のリスト
void LogError(string functionName, int Result, unsigned int ErrorCode);
void LogInfo(string message);
void SeparatorLine();
void PrintSettings();
void PrintHeader();
void PrintUsage();
void SetDefaultParameters();
int OpenDevice(DWORD* ptr_pErrorCode);
void selectOperationMode();
int CloseDevice(DWORD* ptr_pErrorCode);

int Setup(unsigned int* ErrorCode);
bool GetEnc(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int& pErrorCode);
bool ReadState(HANDLE p_DeviceHandle, unsigned short p_usNodeId, int& p_VelocityIs, int& p_PositionIs, short& p_CurrentIs, unsigned int& p_rlErrorCode);
bool DriveVelocityMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int& p_rlErrorCode, long velocity);
bool DrivePositionMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int& p_rlErrorCode, long double mode_val, unsigned int pos_vel);
int StartMotor(unsigned int* p_pErrorCode, long velocity);
void exit_callback_handler(int signum);


void handleProfilePositionMode();
void handleProfileVelocityMode();

// 関数定義
//エラー情報を出力する
void LogError(string functionName, int Result, unsigned int ErrorCode)
{
	cerr << programName << ": " << functionName << " failed (result=" << Result << ", errorCode=0x" << std::hex << ErrorCode << ")" << endl;
}

//情報を出力する
void LogInfo(string message)
{
	cout << message << endl;
}

//取り消し線を印刷する
void SeparatorLine()
{
	const int lineLength = 65;
	for (int i = 0; i < lineLength; i++)
	{
		cout << "-";
	}
	cout << endl;
}

//デバイス情報を画面に出力します。
void PrintSettings()
{
	stringstream msg;

	msg << "Default settings:" << endl;
	msg << "Node id             = " << NodeId << endl;
	msg << "Device name         = " << deviceName << endl;
	msg << "Protocal stack name = " << protocolStackName << endl;
	msg << "Interface name      = " << interfaceName << endl;
	msg << "Port name           = " << portName << endl;
	msg << "Baudrate            = " << baudrate << endl;


	LogInfo(msg.str());
	SeparatorLine();
}
//タイトルを印刷する
void PrintHeader()
{
	SeparatorLine();

	LogInfo("Drive Motor");

	SeparatorLine();
}

//使用説明書を印刷する
void PrintUsage()
{
	SeparatorLine();
	cout << "Usage: ./drive_motor option  argument_values\n";
	cout << "Available Options : \n";
	cout << "\tvel : Velocity Mode (Enter RPM between ??? and ???)" << endl;
	cout << "\tpos : Position Mode (Enter number of rotations of shaft)" << endl;
	cout << "To go back to 0 position, use 0 as value " << endl;
	SeparatorLine();
}

//デフォルトパラメータを設定する
void SetDefaultParameters()
{
	NodeId = 1;
	deviceName = "EPOS4";
	protocolStackName = "MAXON SERIAL V2";
	interfaceName = "USB";
	portName = "USB0";
	baudrate = 1000000;
}

//デバイスを開きます
int OpenDevice(DWORD* ptr_pErrorCode)
{
	
	char* pDeviceName = new char[255];
	char* pProtocolStackName = new char[255];
	char* pInterfaceName = new char[255];
	char* pPortName = new char[255];

	strcpy(pDeviceName, deviceName.c_str());
	strcpy(pProtocolStackName, protocolStackName.c_str());
	strcpy(pInterfaceName, interfaceName.c_str());
	strcpy(pPortName, portName.c_str());

	LogInfo("Open device...");

	KeyHandle = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, ptr_pErrorCode);

	if (KeyHandle != 0 && *ptr_pErrorCode == 0)
	{
		DWORD currentBaudRate = 0;
	    DWORD currentTimeout = 0;

		BOOL success = VCS_GetProtocolStackSettings(KeyHandle, &currentBaudRate, &currentTimeout, ptr_pErrorCode);
		if (success)
		{
			cout << endl;
			cout << "Current baud rate: " << currentBaudRate << " bit/s" << endl;
			cout << "Current timeout: " << currentTimeout << " ms" << endl;

			DWORD newBaudRate = currentBaudRate;
			DWORD newTimeout = currentTimeout;

			success = VCS_SetProtocolStackSettings(KeyHandle, newBaudRate, newTimeout, ptr_pErrorCode);

			if (success)
			{
				cout << "Protocol stack settings updated successfully!" << endl;
			}
			else
			{
				cerr << "Failed to set protocol stack settings, error code: " << ptr_pErrorCode << endl;
			}
		}
		else
		{
			cerr << "Failed to get protocol stack settings, error code: " << ptr_pErrorCode << endl;
		}
	}
	   else
	{
		// Failed to open the device, handle the error here
		cerr << "Failed to open the device, error code: " << ptr_pErrorCode << endl;
		}
}

//動作モードの選択
void selectOperationMode()
{
	if (KeyHandle != 0)
	{
		int operationMode;

		while (true)
		{
			cout << "\n";
			cout << "Select operation mode (1: PPM, 3: PVM, 6: HM, 7: IPM, -1: PM, -2: VM, -3: CM, -5: MEM, -6: SDM): ";
			cin >> operationMode;

			BOOL success = VCS_SetOperationMode(KeyHandle, NodeId, operationMode, &pErrorCode);

			if (success)
			{
				system("CLS");
				std::cout << "Operation mode set successfully!" << std::endl;
				std::cout << "Begin Mode setting" << std::endl;

				switch (operationMode)
				{
				case 1: // PPM
					handleProfilePositionMode();
					break;
				case 3: // PVM
					handleProfileVelocityMode();
					break;
				case 6: // HM
					//handleHomingMode();
					break;
				case 7: // IPM
					//handleInterpolatedPositionMode();
					break;
				case -1: // PM
					//handlePositionMode();
					break;
				case -2: // VM
					//handleVelocityMode();
					break;
				case -3: // CM
					//handleCurrentMode();
					break;
				case -5: // MEM
					//handleMasterEncoderMode();
					break;
				case -6: // SDM
					//handleStepDirectionMode();
					break;
				}

				__int8 currentMode;
				success = VCS_GetOperationMode(KeyHandle, NodeId, &currentMode, &pErrorCode);
				if (success)
					std::cout << "Current operation mode: " << (int)currentMode << std::endl;
				else
					std::cerr << "Failed to get operation mode, error code: " << pErrorCode << std::endl;
			}
			else
			{
				std::cerr << "Failed to set operation mode, error code: " << pErrorCode << std::endl;
			}
		}
	}
}


// デバイスを閉じます。
int CloseDevice(DWORD* ptr_pErrorCode)
{
	if (KeyHandle != nullptr)
	{
		DWORD pErrorCode = 0;

		// Ask the user for the choice to close the device
		int closeChoice;
		cout << "\n";
		cout << "Choose the function to close the device:\n";
		cout << "1. VCS_CloseDevice (close only the current device)\n";
		cout << "2. VCS_CloseAllDevices (close all opened devices)\n";
		cout << "Enter your choice (1 or 2): ";

		while (!(cin >> closeChoice) || (closeChoice != 1 && closeChoice != 2))
		{
			cin.clear();
			cerr << "Invalid input. Please enter 1 or 2: ";
		}

		BOOL closeSuccess = false;

		if (closeChoice == 1)
		{
			closeSuccess = VCS_CloseDevice(KeyHandle, ptr_pErrorCode);
		}
		else if (closeChoice == 2)
		{
			closeSuccess = VCS_CloseAllDevices(ptr_pErrorCode);
		}

		if (closeSuccess)
		{
			LogInfo("The device(s) closed successfully!");
		}
		else
		{
			cerr << "Failed to close device(s), error code: " << ptr_pErrorCode << endl;
		}

		// Clear device handle
		KeyHandle = 0;
	}
	else
	{
		LogInfo("Device is not open, nothing to close.") ;
	}
}

// --------------------------------------------------------------------------------------------------------------------
//                        Basic Motion Control Functions (モーションコントロールの基本機能)
// --------------------------------------------------------------------------------------------------------------------

// コンソール上にモーターAとDの角度をそれぞれ表示する。角度の表記は０➡３６０度とする。
float convertPositionToAngle(long position)
{
	return static_cast<float>(-1) * (position) / (encoderCountsPerRevolution ) * 360;
}
void printCurrentPosition()
{
	long currentPosition = 0;
	BOOL success = VCS_GetPositionIs(KeyHandle, NodeId, &currentPosition, &pErrorCode);

	if (success)
	{
		float currentAngle = convertPositionToAngle(currentPosition);
		cout << "Current position: " << currentAngle << " degrees" << endl;
	}
	else
	{
		cerr << "Failed to get current position, error code: " << pErrorCode << endl;
	}
}


void printCurrentVelocity()
{
	long currentVelocity = 0;
	BOOL success = VCS_GetVelocityIs(KeyHandle, NodeId, &currentVelocity, &pErrorCode);

	if (success)
	{
		cout << "Current velocity: " << currentVelocity << " rpm" << endl;
	}
	else
	{
		cerr << "Failed to get current position, error code: " << pErrorCode << endl;
	}
}


// 関数：モーターAとモーターDの角度差を計算

void calculateAngleDifference() {
	float currentAngleA=0;
	float currentAngleD=0;

	double angleDifference = currentAngleA - currentAngleD;
	cout << "Angle Difference (A - D): " << angleDifference << " degrees" << endl;
}

void ControlTorqueBasedOnAnalogInput() {
	// アナログ入力に応じてモーターAがトルクを制御するコード
	double analogInput = 0; // アナログ入力値を取得（仮定）
	double torque = 0; // 計算されたトルク

	// アナログ入力に応じてトルクを計算（例：アナログ入力が高いほどトルクを増加）
	torque = analogInput * 50 * 52.5; // Torque constant = 52.5 mNm/A; Analog input constant = 50mA/V

	cout << "CurrentTorque: " << torque << " mNm" << endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100ミリ秒待つ
}


void handleProfilePositionMode()
{

	DWORD errorCode = 0;

	// Activate Profile Position Mode
	BOOL success = VCS_ActivateProfilePositionMode(KeyHandle, NodeId, &pErrorCode);

	if (success) {
		std::cout << "\n";
		std::cout << "Profile Position Mode activated." << std::endl;
	}
	else {
		std::cerr << "Failed to activate Profile Position Mode, error code: " << pErrorCode << std::endl;
		return;
	}

	BOOL enabled = VCS_SetEnableState(KeyHandle, NodeId, &pErrorCode);
	if (enabled) {
		std::cout << "Device Enabled" << std::endl;
	}
	else {
		std::cerr << "Failed to Enable Device, error code: " << errorCode << std::endl;
		return;
	}




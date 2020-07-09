#include "Touchpad3DDevice.h"

using namespace std;

Touchpad3DDevice::Touchpad3DDevice() : m_device(nullptr), isTouching(false), freezeDataAcquired(false), m_stop(false), synced(false)
{
}

Touchpad3DDevice::~Touchpad3DDevice()
{
	stop();
	/* Do the cleanup work */
	free_device();
//	m_PressureConsole.Close();
}

void Touchpad3DDevice::run()
{
//	m_PressureConsole.Create("Sensor Console");
//	m_PressureConsole.setupScreen();
	/* Initialize data and the device */
	init_data();
	init_device();

	isTouching = false;

	/* Retrieve all 2D events and states */
	while (m_data.running && !m_stop)
	{
		if (!freezeDataAcquired)
		{
			update_device();
			if (m_data.fingers->count != 0) // || m_data.out_touch->touch_flags)
			{
				isTouching = true;
			}
			else {
				isTouching = false;
			}
		}
		Sleep(10);
	}

	m_data.to_file_2d = ("Sync information: global sync logger time " +
		m_data.global_sync_logger_time + " equals " + std::to_string(m_data.local_sync_dfs) + "\n" +
		m_data.to_file_2d);
}

void Touchpad3DDevice::stop()
{
	m_stop = true;
}

void Touchpad3DDevice::init_device()
{
	int result;

	//const int stream_flags = hmi3d_DataOutConfigMask_TouchInfo;
	const int com_flags = (hmi2d_com_raw_self_measure | hmi2d_com_raw_mutual_measure |
						   hmi2d_com_cal_self_measure | hmi2d_com_cal_mutual_measure |
						   hmi2d_com_finger_positions);

	/* Create and initialize HMI instance */
	m_device = hmi_create();
	if (!m_device)
	{
		int msgboxID = MessageBox(
			NULL,
			"Could not create a 3D Touch Pad interface instance!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}

	hmi_cleanup(m_device);
    //Sleep(2000);
	/* Initialize the hmi_t-instance */
	hmi_initialize(m_device);

	/* Get pointers to the result-buffers */
    m_data.raw = hmi2d_get_mutual_cal(m_device);

	/* Open connection to device */
	result = hmi_open(m_device);
	if (result != HMI_NO_ERROR) {
		int msgboxID = MessageBox(
			NULL,
			"Could not open connection to device!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}

	//m_data.out_touch = hmi3d_get_touch(m_device);
	m_data.fingers = hmi2d_get_finger_positions(m_device);

	/* Set required communication mode */
	result = hmi2d_set_com_mask(m_device, (hmi2d_com_mask_t)com_flags, hmi2d_com_all);
	if (result != HMI_NO_ERROR)
	{
		int msgboxID = MessageBox(
			NULL,
			"Could not set communication mode between PC and 3D Touch Sensor!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}

	/* Set required active features */
	result = hmi2d_set_active_mask(m_device, hmi2d_active_full_mutual, hmi2d_active_all);
	if (result != HMI_NO_ERROR)
	{
		int msgboxID = MessageBox(
			NULL,
			"Could not get 2D Touch Sensor Data!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}

	/* Set 2D/3D mixed operation mode */
	result = hmi2d_set_operation_mode(m_device, hmi2d_2d_mode);
	if (result != HMI_NO_ERROR) {
		int msgboxID = MessageBox(
			NULL,
			"Could not set 2D/3D mixed operation mode!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}

	/* Reset the device to the default state:
	 * - Automatic calibration enabled
	 * - All frequencies allowed
	 * - Approach detection enabled for power saving
	 * - Enable all gestures ( bit 1 to 6 set to 1 = 126 )
	 */
	/*m_data.air_wheel = 1;
	m_data.touch_detect = 1;
	if (hmi3d_set_auto_calibration(m_device, m_data.auto_calib) < 0 ||
		hmi3d_select_frequencies(m_device, hmi3d_all_freq) < 0 ||
		hmi3d_set_touch_detection(m_device, m_data.touch_detect) < 0)
	{
		int msgboxID = MessageBox(
			NULL,
			"Could not reset/calibrate device to default state!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}*/

	/* Set output-mask to the bitmask defined aboveand stream only changes */
	/*result = hmi3d_set_output_enable_mask(m_device, (hmi3d_DataOutConfigMask_t)stream_flags, (hmi3d_DataOutConfigMask_t)0, hmi3d_DataOutConfigMask_OutputAll);
	if (result != HMI_NO_ERROR)
	{
		int msgboxID = MessageBox(
			NULL,
			"Could not start data-streaming from device!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}*/
}

void Touchpad3DDevice::free_device()
{
	/* Reset default communication mode */
	if (hmi2d_set_com_mask(m_device, hmi2d_com_default, hmi2d_com_all)
		!= HMI_NO_ERROR)
	{
		int msgboxID = MessageBox(
			NULL,
			"Could not reset default communication mode!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}

	/* Reset default features */
	if (hmi2d_set_active_mask(m_device, hmi2d_active_default, hmi2d_active_all)
		!= HMI_NO_ERROR)
	{
		int msgboxID = MessageBox(
			NULL,
			"Could not reset default active mask.!",
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}

	/* Close the connection to the device */
	hmi_close(m_device);

	/* Release resources that were associated with the hmi_t-instance */
	hmi_cleanup(m_device);

	/* Release the hmi_t-instance itself */
	hmi_free(m_device);
}

void Touchpad3DDevice::init_data()
{
	/* The demo should be running */
	m_data.running = 1;
	/* Automatic calibration enabled by default */
	m_data.auto_calib = 1;

	m_data.local_start_time = chrono::steady_clock::now();
}

void Touchpad3DDevice::syncWithGlobal(string & gst)
{
	m_data.global_sync_logger_time = gst;
	m_data.local_sync_dfs = getElapsedMilliSecsSinceStart();
	synced = true;
}

void Touchpad3DDevice::update_device()
{
	//int rawData[12][16] = { 0 };

	/* Retrieve all 3D events and states */
	/*int result = hmi3d_retrieve_data(m_device, NULL);
	if (result != HMI_NO_ERROR && result != HMI_NO_DATA)
	{
		std::string errorMessage = "Error while retrieving 3D Data from the device! Error code: " + std::to_string(result);
		int msgboxID = MessageBox(
			NULL,
			errorMessage.c_str(),
			"Sensor Manager",
			MB_ICONWARNING | MB_OK
		);
	}
    else if (result == HMI_NO_ERROR && !m_stop) {
        // got the data
	}*/

	/* Retrieve all 2D events and states */
	int result = hmi2d_retrieve_data(m_device);
	if (result != HMI_NO_ERROR && result != HMI_NO_DATA) {
		std::string errorMessage = "Error while retrieving 2D Data from the device! Error code: " + std::to_string(result);
    	int msgboxID = MessageBox(
    		NULL,
    		errorMessage.c_str(),
    		"Sensor Manager",
    		MB_ICONWARNING | MB_OK
    	);
    }
    else if (result == HMI_NO_ERROR && !m_stop) {
        // got the data

		// fill the buffer
        const int nRows = 12, nColumns = 16;
        int32_t rawDataBuf[nRows][nColumns] = { 0 };

        /*for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nColumns; j++) {
                rawDataBuf[i][j] = (*m_data.raw)[i][j];
            }
        }*/

        //In case the buffer gets underfilled try this:
		while (result != HMI_NO_DATA) {
			for (int i = 0; i < nRows; i++) {
				bool allZeros = true;  // if the row is zeros then it did not change
				for (int j = 0; j < nColumns; j++) {
					if ((*m_data.raw)[i][j] != 0) {
						allZeros = false;
						break;
					}
				}
				if (!allZeros) {
					for (int j = 0; j < nColumns; j++) {
						rawDataBuf[i][j] = (*m_data.raw)[i][j];
					}
				}
			}
			result = hmi2d_retrieve_data(m_device);
		}

		// save to buffer
		m_data.to_file_2d += std::to_string(getElapsedMilliSecsSinceStart()) + ": ";

		for (int i = 0; i < nRows; ++i) {
			for (int j = 0; j < nColumns; ++j) {
				m_data.to_file_2d += to_string(rawDataBuf[i][j]) + " ";
			}
			m_data.to_file_2d += "|";
		}
		m_data.to_file_2d += "\n";
	}
}

long Touchpad3DDevice::getElapsedMilliSecsSinceStart()
{
	auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() -
															   m_data.local_start_time);
	return (long)elapsed.count();
}

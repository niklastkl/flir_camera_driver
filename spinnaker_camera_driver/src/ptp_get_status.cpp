#include "spinnaker_camera_driver/ptp_config.h"

int main(int argc, char** argv){
    auto camInterface = SpinnakerCamInterface();
    camInterface.PrintSpinnakerVersion();
    bool result =  camInterface.CheckPTPSyncStatus();
    if (!result){
        std::cout << "[PTP_CONFIG] Fetching PTP status for all connected cameras failed" << std::endl;
        return -1;
    }
    return 0;
}
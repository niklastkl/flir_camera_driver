#include "spinnaker_camera_driver/ptp_config.h"

int main(int argc, char** argv){
    auto camInterface = SpinnakerCamInterface();
    bool result =  camInterface.DeactivatePTP();
    if (!result){
        std::cout << "[PTP_CONFIG] Deactivating PTP failed for all connected cameras failed" << std::endl;
        return -1;
    }
    return 0;
}
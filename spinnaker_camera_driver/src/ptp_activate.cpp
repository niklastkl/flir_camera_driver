#include "spinnaker_camera_driver/ptp_config.h"

int main(int argc, char** argv){
    auto camInterface = SpinnakerCamInterface();
    bool result =  camInterface.ActivatePTP();
    if (!result){
        std::cout << "[PTP_CONFIG] Activating PTP failed for all connected cameras failed" << std::endl;
        return -1;
    }
    return 0;
}
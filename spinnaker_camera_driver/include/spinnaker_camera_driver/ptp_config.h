#include "Spinnaker.h"
#include <functional>

namespace ptp_config{

    bool SetPTPStatus(Spinnaker::CameraPtr pCam, bool activate_ptp);

    bool CheckPTPSyncStatus(Spinnaker::CameraPtr pCam, int max_attempts=10);

}

class SpinnakerCamInterface{
    public:
        SpinnakerCamInterface(int max_connection_retries=3);
        
        ~SpinnakerCamInterface();

        void Init(int max_connection_retries);

        void PrintSpinnakerVersion();

        bool ActivatePTP();

        bool DeactivatePTP();

        bool CheckPTPSyncStatus();


    private:

        bool ConfigCamera(std::function<bool (Spinnaker::CameraPtr)> func);

        Spinnaker::SystemPtr systemPtr_;
        Spinnaker::InterfaceList interfaceList_;
        Spinnaker::CameraList camList_;

};


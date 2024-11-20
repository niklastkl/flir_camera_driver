#include "spinnaker_camera_driver/ptp_config.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <boost/bind.hpp>
#include <chrono>
#include <thread>


// Funcitonality:

namespace ptp_config{

    bool SetPTPStatus(Spinnaker::CameraPtr pCam, bool activate_ptp)
    {
        std::string status = activate_ptp ? "on" : "off";
        bool result = true;
            std::cout << "Try to set PTP to " << status << std::endl;
        try{
            if (!pCam){
                std::cout << "Camera is not initialized yet, cannot configure PTP." << std::endl;
                return false;
            }
            // Enable PTP settings
            Spinnaker::GenApi::CBooleanPtr ptrIEEE1588 = pCam->GetNodeMap().GetNode("GevIEEE1588");
            if (!Spinnaker::GenApi::IsAvailable(ptrIEEE1588) || !IsWritable(ptrIEEE1588))
            {
                std::cout << "Camera Unable to enable PTP (node retrieval). Aborting..." << std::endl;
                return false;
            }

            if (ptrIEEE1588->GetValue() != activate_ptp){
                // Disable PTP
                ptrIEEE1588->SetValue(activate_ptp);
                std::cout << "Set PTP to " << status << "." << std::endl;
                if (activate_ptp){
                    std::cout << "Wait 10 seconds to configure" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                }
            }
            else{
                std::cout << "PTP was already set to " << status << "." << std::endl;
            }
        }
        
        catch (Spinnaker::Exception& e)
        {
            std::cout << "Error: " << e.what() << std::endl;
            result = false;
        }
        return result;
    }

    bool CheckPTPSyncStatus(Spinnaker::CameraPtr pCam, int max_attempts)
    {
        bool result = true;
        try{
            if (!pCam){
                std::cout << "Camera is not initialized yet, cannot configure PTP." << std::endl;
                return false;
            }
            // Get node to latch PTP data
            Spinnaker::GenApi::CCommandPtr ptrGevIEEE1588DataSetLatch = pCam->GetNodeMap().GetNode("GevIEEE1588DataSetLatch");
            if (!Spinnaker::GenApi::IsAvailable(ptrGevIEEE1588DataSetLatch))
            {
                std::cout << "Camera Unable to execute PTP data set latch (node retrieval). Aborting..."
                    << std::endl;
            }

            ptrGevIEEE1588DataSetLatch->Execute();

            // Check if PTP status is not in intialization
            Spinnaker::GenApi::CEnumerationPtr ptrGevIEEE1588StatusLatched = pCam->GetNodeMap().GetNode("GevIEEE1588StatusLatched");
            if (!Spinnaker::GenApi::IsAvailable(ptrGevIEEE1588StatusLatched) || !Spinnaker::GenApi::IsReadable(ptrGevIEEE1588StatusLatched))
            {
                std::cout << "Camera Unable to read PTP status (node retrieval). Aborting..." << std::endl;
                return false;
            }

            Spinnaker::GenApi::CEnumEntryPtr ptrGevIEEE1588StatusLatchedInitializing =
                ptrGevIEEE1588StatusLatched->GetEntryByName("Initializing");
            if (!Spinnaker::GenApi::IsAvailable(ptrGevIEEE1588StatusLatchedInitializing) ||
                !Spinnaker::GenApi::IsReadable(ptrGevIEEE1588StatusLatchedInitializing))
            {
                std::cout << "Camera Unable to get PTP status (enum entry retrieval). Aborting..." << std::endl;
                return false;
            }

            int attempts = 0;
            while (ptrGevIEEE1588StatusLatched->GetIntValue() == ptrGevIEEE1588StatusLatchedInitializing->GetValue() && attempts < max_attempts)
            {
                attempts += 1;
                std::cout << "Camera still in Initializing mode, retrying..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                ptrGevIEEE1588DataSetLatch->Execute();
            }
            // todo: print here, in which mode the clock actually is in => need to fetch the other modes as well

            if (attempts == max_attempts){
                std::cout << "Camera in Initializing mode too long, abort." << std::endl;
                return false;
            }
            
            // Check if camera(s) is(are) synchronized to master camera
            // Verify if camera offset from master is larger than 1000ns which means camera(s) is(are) not synchronized
            Spinnaker::GenApi::CIntegerPtr ptrGevIEEE1588OffsetFromMasterLatched = pCam->GetNodeMap().GetNode("GevIEEE1588OffsetFromMasterLatched");
            if (!Spinnaker::GenApi::IsAvailable(ptrGevIEEE1588OffsetFromMasterLatched) ||
                !Spinnaker::GenApi::IsReadable(ptrGevIEEE1588OffsetFromMasterLatched))
            {
                std::cout << "Camera Unable to read PTP offset (node retrieval). Aborting..." << std::endl;
                return false;
            }

            attempts = 0;
            while (std::abs(ptrGevIEEE1588OffsetFromMasterLatched->GetValue()) > 1000 && attempts < max_attempts)
            {
                attempts += 1;
                std::cout << "Offset from Master: " << ptrGevIEEE1588OffsetFromMasterLatched->GetValue() << " \n Wait until is below 1000ns" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                ptrGevIEEE1588DataSetLatch->Execute();
            }
            if (attempts == max_attempts)
            {
                std::cout << "Camera still has offset higher than 1000ns. Camera(s) is(are) not synchronized" << std::endl;
                return false;
            }

            std::cout << "Offset from Master: " << int(ptrGevIEEE1588OffsetFromMasterLatched->GetValue()) << std::endl;
        }
        
        catch (Spinnaker::Exception& e)
        {
            std::cout << "Error: " << e.what() << std::endl;
            result = false;
        }
        return result;
    }
}

SpinnakerCamInterface::SpinnakerCamInterface(){
    Init();
}

SpinnakerCamInterface::~SpinnakerCamInterface(){
    camList_.Clear();
    interfaceList_.Clear();
    systemPtr_->ReleaseInstance();
}   

void SpinnakerCamInterface::Init(){
    systemPtr_ = Spinnaker::System::GetInstance();

    interfaceList_ = systemPtr_->GetInterfaces();
    unsigned int numInterfaces = interfaceList_.GetSize();
    std::printf("\033[93m[Spinnaker] Number of interfaces detected: %d \n", numInterfaces);

    camList_ = systemPtr_->GetCameras();
    unsigned int numCameras = camList_.GetSize();

    std::printf("\033[93m[Spinnaker] # of connected cameras: %d \n", numCameras);
    
    // Finish if there are no cameras
    if (numCameras == 0)
    {
        std::printf("\033[91mNO Cameras Connected! \n\n");
        return;
    }
    for (unsigned int i = 0; i < numCameras; i++)
    {
        Spinnaker::CameraPtr pCam = camList_[i];
        Spinnaker::GenApi::INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
        Spinnaker::GenApi::CStringPtr ptrDeviceSerialNumber = nodeMapTLDevice.GetNode("DeviceSerialNumber");
        if (Spinnaker::GenApi::IsAvailable(ptrDeviceSerialNumber) && Spinnaker::GenApi::IsReadable(ptrDeviceSerialNumber))
        {
            std::cout << "\033[92m[" << i << "]\t" << ptrDeviceSerialNumber->ToString() << std::endl;
        }
    }
}

void SpinnakerCamInterface::PrintSpinnakerVersion(){
    const Spinnaker::LibraryVersion spinnakerLibraryVersion = systemPtr_->GetLibraryVersion();
    std::cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
            << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << std::endl;
            
}

bool SpinnakerCamInterface::ActivatePTP(){
    std::cout << "Activate PTP" << std::endl;
    return ConfigCamera(boost::bind(&ptp_config::SetPTPStatus, _1, true));
}

bool SpinnakerCamInterface::DeactivatePTP(){
    std::cout << "Deactivate PTP" << std::endl;
    return ConfigCamera(boost::bind(&ptp_config::SetPTPStatus, _1, false));
}

bool SpinnakerCamInterface::CheckPTPSyncStatus(){
    std::cout << "check PTP status" << std::endl;
    return ConfigCamera(boost::bind(ptp_config::CheckPTPSyncStatus, _1, 20));
}

bool SpinnakerCamInterface::ConfigCamera(std::function<bool (Spinnaker::CameraPtr)> func){
    bool result = true;
    if (camList_.GetSize() == 0)
    {
        std::printf("\033[91mNO Cameras Connected! \n\n");
        return false;
    }
    
    for (unsigned int i = 0; i < camList_.GetSize(); i++)
    {
        Spinnaker::CameraPtr pCam = camList_[i];
        pCam->Init();
        result &= func(pCam);
        pCam->DeInit();
    }
    return result;
}



/**
 *
 * This example demonstrates how to get images from the robot remotely, how
 * to track a blob using all the four joints of the Romeo Head;
 *
 */

// Aldebaran includes.
#include <alproxies/altexttospeechproxy.h>
#include <alproxies/alsystemproxy.h>

// ViSP includes.
#include <visp/vpDisplayX.h>
#include <visp/vpImage.h>
#include <visp/vpImageConvert.h>

#include <visp/vpDot2.h>
#include <visp/vpImageIo.h>
#include <visp/vpImagePoint.h>
#include <visp/vpFeaturePoint.h>
#include <visp/vpServo.h>
#include <visp/vpCameraParameters.h>
#include <visp/vpPixelMeterConversion.h>
#include <visp/vpMeterPixelConversion.h>
#include <visp/vpPlot.h>
#include <visp/vpFeatureBuilder.h>
#include <visp/vpXmlParserCamera.h>
#include <visp/vpXmlParserHomogeneousMatrix.h>
#include <visp/vpPose.h>

#include <iostream>
#include <string>
#include <list>
#include <iterator>

#include <visp_naoqi/vpNaoqiGrabber.h>
#include <visp_naoqi/vpNaoqiRobot.h>




#include <vpRomeoTkConfig.h>


#define SAVE 0

#define USE_PLOTTER
#define L 0.015


using namespace AL;



int main(int argc, char* argv[])
{
  std::string robotIp = "198.18.0.1";

  if (argc < 2) {
    std::cerr << "Usage: almotion_setangles robotIp "
              << "(optional default \"198.18.0.1\")."<< std::endl;
  }
  else {
    robotIp = argv[1];
  }


  /** Open the grabber for the acquisition of the images from the robot*/
  vpNaoqiGrabber g;
  g.open();

  /** Create a new istance NaoqiRobot*/
  vpNaoqiRobot robot;
  robot.open();



  /** Initialization Visp Image, display and camera paramenters*/

  vpImage<unsigned char> I(g.getHeight(), g.getWidth());
  vpDisplayX d(I);
  vpDisplay::setTitle(I, "ViSP viewer");
  vpCameraParameters cam_ros;
  cam_ros.initPersProjWithoutDistortion(342.82,342.60,174.552518, 109.978367);


  char filename[FILENAME_MAX];
  vpCameraParameters cam;
  vpXmlParserCamera p; // Create a XML parser
  vpCameraParameters::vpCameraParametersProjType projModel; // Projection model
  // Use a perspective projection model without distortion
  projModel = vpCameraParameters::perspectiveProjWithDistortion;
  // Parse the xml file "myXmlFile.xml" to find the intrinsic camera
  // parameters of the camera named "myCamera" for the image sizes 640x480,
  // for the projection model projModel. The size of the image is optional
  // if camera parameters are given only for one image size.
  sprintf(filename, "%s", "camera.xml");
  if (p.parse(cam, filename, "Camera", projModel, I.getWidth(), I.getHeight()) != vpXmlParserCamera::SEQUENCE_OK) {
    std::cout << "Cannot found camera parameters in file: " << filename << std::endl;
    std::cout << "Loading default camera parameters" << std::endl;
    cam.initPersProjWithoutDistortion(342.82, 342.60, 174.552518, 109.978367);
  }

  std::cout << "Camera parameters: " << cam << std::endl;

  // Constant transformation Target Frame to LArm end-effector (LWristPitch)
  vpHomogeneousMatrix oMe_LArm;
//  for(unsigned int i=0; i<3; i++)
//    oMe_LArm[i][i] = 0; // remove identity
//  oMe_LArm[0][0] = 1;
//  oMe_LArm[1][2] = 1;
//  oMe_LArm[2][1] = -1;

//  oMe_LArm[0][3] = -0.045;
//  oMe_LArm[1][3] = -0.04;
//  oMe_LArm[2][3] = -0.045;


  std::string filename_transform = std::string(ROMEOTK_DATA_FOLDER) + "/transformation.xml";
  std::string name_transform = "qrcode_M_e_LArm";
  {
    vpXmlParserHomogeneousMatrix pm; // Create a XML parser

    if (pm.parse(oMe_LArm, filename_transform, name_transform) != vpXmlParserHomogeneousMatrix::SEQUENCE_OK) {
      std::cout << "Cannot found the homogeneous matrix named " << name_transform << "." << std::endl;
      return 0;
    }
    else
      std::cout << "Homogeneous matrix " << name_transform <<": " << std::endl << oMe_LArm << std::endl;
  }







  // Introduce a matrix to pass from camera frame of Aldebaran to visp camera frame
  vpHomogeneousMatrix cam_alMe_camvisp;
  for(unsigned int i=0; i<3; i++)
    cam_alMe_camvisp[i][i] = 0; // remove identity
  cam_alMe_camvisp[0][2] = 1.;
  cam_alMe_camvisp[1][0] = -1.;
  cam_alMe_camvisp[2][1] = -1.;


  // Motion
  std::vector<std::string> jointNames =  robot.getBodyNames("LArm");
  jointNames.pop_back(); // Delete last joints LHand, that we don't consider in the servo
  const unsigned int numJoints = jointNames.size();

  std::cout << "The " << numJoints << " joints of the Arm:" << std::endl << jointNames << std::endl;

  // Homogeneus matrix from Torso to the Camera (using the sensor of the robot)
  vpHomogeneousMatrix torsoMlcam_visp;
  // Homogeneus matrix from Torso to the Camera (using the estimated extrinsic paramenter)
  vpHomogeneousMatrix torsoMlcam_visp_est;
  //Set the stiffness
  robot.setStiffness(jointNames, 1.f);

  vpImage<vpRGBa> O;


  while(1)
  {


#if 0
    showImages(camProxy,clientName, I);
    if(vpDisplay::getClick(I, false)) {
      vpImageIo::write(I, "/tmp/I.png");
    }
  }
#else
    try
    {
      g.acquire(I);
      vpDisplay::display(I);


      // get the torsoMlcam_al tranformation from NaoQi api
      vpHomogeneousMatrix torsoMlcam_al(robot.getProxy()->getTransform("CameraLeft", 0, false));
      torsoMlcam_visp = torsoMlcam_al * cam_alMe_camvisp;


      vpHomogeneousMatrix torsoMHeadRoll(robot.getProxy()->getTransform("HeadRoll", 0, false));
      std::cout << "torsoMHeadRoll:\n" << torsoMHeadRoll << std::endl;


      // Compute torsoMlcam_visp using the estimated extrinsic camera paramenters

//      std::ifstream f("eMc.dat") ;
//      eMc.load(f) ;
//      f.close() ;

      vpHomogeneousMatrix eMc = g.get_eMc();

      std::cout << "eMc:\n" << eMc << std::endl;
      torsoMlcam_visp_est = torsoMHeadRoll * eMc;

      vpHomogeneousMatrix HeadRollMcam_visp;
      HeadRollMcam_visp = torsoMHeadRoll.inverse()* torsoMlcam_visp ;
      std::cout << "HeadRoll M camera visp:\n" << HeadRollMcam_visp << std::endl;


      std::cout << "torsoMlcam_visp_est:\n" << torsoMlcam_visp_est << std::endl;
      std::cout << "torso M camera visp:\n" << torsoMlcam_visp << std::endl;


      //############################################################################################################
      //                                                  LARM
      //############################################################################################################
      // LWristPitch tranformation -----------------------------------------------------

      vpHomogeneousMatrix torsoMLWristPitch( robot.getProxy()->getTransform("LWristPitch", 0, true));
      std::cout << "Torso M LWristPitch:\n" << torsoMLWristPitch << std::endl;
      vpDisplay::displayFrame(I, torsoMlcam_visp.inverse()*torsoMLWristPitch, cam, 0.04, vpColor::blue);

      // Using estimated eMc
      vpDisplay::displayFrame(I, torsoMlcam_visp_est.inverse()*torsoMLWristPitch, cam, 0.07, vpColor::none);
      // Using estimated eMc and cam_ros
      //vpDisplay::displayFrame(I, torsoMlcam_visp_est.inverse()*torsoMLWristPitch, cam_ros, 0.04, vpColor::yellow);


      // Target estimation from sensor
      vpDisplay::displayFrame(I, torsoMlcam_visp_est.inverse()*torsoMLWristPitch*oMe_LArm.inverse(), cam, 0.04, vpColor::none);

      // -----------------------------------------------------------------------------------




      // LElbowRoll tranformation -----------------------------------------------------
      vpHomogeneousMatrix torsoMLElbowRoll(robot.getProxy()->getTransform("LElbowRoll", 0, true));
      std::cout << "Torso M LElbowRoll:\n" << torsoMLElbowRoll << std::endl;
      vpDisplay::displayFrame(I, torsoMlcam_visp.inverse()*torsoMLElbowRoll, cam, 0.04, vpColor::none);


      // -----------------------------------------------------------------------------------

      // LElbowYaw tranformation -----------------------------------------------------

      vpHomogeneousMatrix torsoMLLElbowYaw(robot.getProxy()->getTransform("LElbowYaw", 0, true));
      std::cout << "Torso M LElbowYaw:\n" << torsoMLLElbowYaw << std::endl;
      vpDisplay::displayFrame(I, torsoMlcam_visp.inverse()*torsoMLLElbowYaw, cam, 0.04, vpColor::none);


      // -----------------------------------------------------------------------------------


      //############################################################################################################
      //                                                  RARM
      //############################################################################################################

      // RWristPitch tranformation -----------------------------------------------------

      vpHomogeneousMatrix torsoMRWristPitch(robot.getProxy()->getTransform("RWristPitch", 0, true));
      std::cout << "Torso M RWristPitch:\n" << torsoMRWristPitch << std::endl;
      vpDisplay::displayFrame(I, torsoMlcam_visp.inverse()*torsoMRWristPitch, cam, 0.04, vpColor::none);


      // -----------------------------------------------------------------------------------

      // RElbowRoll tranformation -----------------------------------------------------

      vpHomogeneousMatrix torsoMRElbowRoll(robot.getProxy()->getTransform("RElbowRoll", 0, true));
      std::cout << "Torso M RElbowRoll:\n" << torsoMRElbowRoll << std::endl;
      vpDisplay::displayFrame(I, torsoMlcam_visp.inverse()*torsoMRElbowRoll, cam, 0.04, vpColor::none);


      // -----------------------------------------------------------------------------------

      // RElbowYaw tranformation -----------------------------------------------------


      vpHomogeneousMatrix torsoMRElbowYaw(robot.getProxy()->getTransform("RElbowYaw", 0, true));
      std::cout << "Torso M RElbowYaw:\n" << torsoMRElbowYaw << std::endl;
      vpDisplay::displayFrame(I, torsoMlcam_visp.inverse()*torsoMRElbowYaw, cam, 0.04, vpColor::none);


      // -----------------------------------------------------------------------------------


      vpDisplay::flush(I) ;
      //vpTime::sleepMs(20);


    }
    catch (const AL::ALError& e)
    {
      std::cerr << "Caught exception " << e.what() << std::endl;
    }

    if (vpDisplay::getClick(I, false))
      break;

    vpDisplay::flush(I);
    vpDisplay::getImage(I, O);

  }

  std::cout << "The end: stop the robot..." << std::endl;

#endif

  return 0;
}


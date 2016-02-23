/****************************************************************************
 *
 * $Id: mbtEdgeTracking.cpp 5535 2015-07-20 13:48:59Z ayol $
 *
 * This file is part of the ViSP software.
 * Copyright (C) 2005 - 2014 by INRIA. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * ("GPL") version 2 as published by the Free Software Foundation.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact INRIA about acquiring a ViSP Professional
 * Edition License.
 *
 * See http://www.irisa.fr/lagadic/visp/visp.html for more information.
 *
 * This software was developed at:
 * INRIA Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * http://www.irisa.fr/lagadic
 *
 * If you have questions regarding the use of this file, please contact
 * INRIA at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * Description:
 * Example of model based tracking.
 *
 * Authors:
 * Nicolas Melchior
 * Romain Tallonneau
 * Aurelien Yol
 *
 *****************************************************************************/

/*!
  \example mbtEdgeTracking.cpp

  \brief Example of model based tracking on an image sequence containing a cube.
*/

#include <iostream>
#include <visp3/core/vpConfig.h>


#include <visp_naoqi/vpNaoqiGrabber.h>

#if defined(VISP_HAVE_MODULE_MBT) && defined (VISP_HAVE_DISPLAY)

#include <visp3/core/vpDebug.h>
#include <visp3/gui/vpDisplayD3D.h>
#include <visp3/gui/vpDisplayGTK.h>
#include <visp3/gui/vpDisplayGDI.h>
#include <visp3/gui/vpDisplayOpenCV.h>
#include <visp3/gui/vpDisplayX.h>
#include <visp3/core/vpHomogeneousMatrix.h>
#include <visp3/io/vpImageIo.h>
#include <visp3/core/vpIoTools.h>
#include <visp3/core/vpMath.h>
#include <visp3/io/vpVideoReader.h>
#include <visp3/io/vpParseArgv.h>
#include <visp3/mbt/vpMbEdgeTracker.h>
#include <visp3/mbt/vpMbEdgeKltTracker.h>
#include <visp3/mbt/vpMbKltTracker.h>
#include <visp3/sensor/vp1394TwoGrabber.h>

#define GETOPTARGS  "x:m:i:n:dchtfCol"

void usage(const char *name, const char *badparam);
bool getOptions(int argc, const char **argv, std::string &ipath, std::string &configFile, std::string &modelFile,
                std::string &initFile, bool &displayFeatures, bool &click_allowed, bool &display,
                bool& cao3DModel, bool& trackCylinder, bool &useOgre, bool &useScanline);

void usage(const char *name, const char *badparam)
{
  fprintf(stdout, "\n\
          Example of tracking based on the 3D model.\n\
          \n\
          SYNOPSIS\n\
          %s [-i <test image path>] [-x <config file>]\n\
      [-m <model name>] [-n <initialisation file base name>]\n\
      [-t] [-c] [-d] [-h] [-f] [-C] [-o] [-l]",
      name );

  fprintf(stdout, "\n\
          OPTIONS:                                               \n\
          -i <input image path>                                \n\
          Set image input path.\n\
          From this path read images \n\
          \"ViSP-images/mbt/cube/image%%04d.ppm\". These \n\
          images come from ViSP-images-x.y.z.tar.gz available \n\
          on the ViSP website.\n\
          Setting the VISP_INPUT_IMAGE_PATH environment\n\
          variable produces the same behaviour than using\n\
          this option.\n\
          \n\
          -x <config file>                                     \n\
          Set the config file (the xml file) to use.\n\
          The config file is used to specify the parameters of the tracker.\n\
          \n\
          -m <model name>                                 \n\
          Specify the name of the file of the model\n\
          The model can either be a vrml model (.wrl) or a .cao file.\n\
          \n\
          -f                                  \n\
          Do not use the vrml model, use the .cao one. These two models are \n\
          equivalent and comes from ViSP-images-x.y.z.tar.gz available on the ViSP\n\
          website. However, the .cao model allows to use the 3d model based tracker \n\
          without Coin.\n\
          \n\
          -C                                  \n\
          Track only the cube (not the cylinder). In this case the models files are\n\
          cube.cao or cube.wrl instead of cube_and_cylinder.cao and \n\
          cube_and_cylinder.wrl.\n\
          \n\
          -n <initialisation file base name>                                            \n\
          Base name of the initialisation file. The file will be 'base_name'.init .\n\
          This base name is also used for the optionnal picture specifying where to \n\
          click (a .ppm picture).\
          \n\
          -t \n\
          Turn off the display of the the moving edges. \n\
          \n\
          -d \n\
          Turn off the display.\n\
          \n\
          -c\n\
          Disable the mouse click. Useful to automaze the \n\
          execution of this program without humain intervention.\n\
          \n\
          -o\n\
          Use Ogre3D for visibility tests\n\
          \n\
          -l\n\
          Use the scanline for visibility tests\n\
          \n\
          -h \n\
          Print the help.\n\n");

          if (badparam)
          fprintf(stdout, "\nERROR: Bad parameter [%s]\n", badparam);
}


bool getOptions(int argc, const char **argv, std::string &ipath, std::string &configFile, std::string &modelFile,
                std::string &initFile, bool &displayFeatures, bool &click_allowed, bool &display,
                bool& cao3DModel, bool& trackCylinder, bool &useOgre, bool &useScanline)
{
  const char *optarg_;
  int   c;
  while ((c = vpParseArgv::parse(argc, argv, GETOPTARGS, &optarg_)) > 1) {

    switch (c) {
    case 'i': ipath = optarg_; break;
    case 'x': configFile = optarg_; break;
    case 'm': modelFile = optarg_; break;
    case 'n': initFile = optarg_; break;
    case 't': displayFeatures = false; break;
    case 'f': cao3DModel = true; break;
    case 'c': click_allowed = false; break;
    case 'd': display = false; break;
    case 'C': trackCylinder = false; break;
    case 'o': useOgre = true; break;
    case 'l': useScanline = true; break;
    case 'h': usage(argv[0], NULL); return false; break;

    default:
      usage(argv[0], optarg_);
      return false; break;
    }
  }

  if ((c == 1) || (c == -1)) {
    // standalone param or error
    usage(argv[0], NULL);
    std::cerr << "ERROR: " << std::endl;
    std::cerr << "  Bad argument " << optarg_ << std::endl << std::endl;
    return false;
  }

  return true;
}

int
main(int argc, const char ** argv)
{
  try {
    std::string env_ipath;
    std::string opt_ipath;
    std::string ipath;
    std::string opt_configFile;
    std::string configFile;
    std::string opt_modelFile;
    std::string modelFile;
    std::string opt_initFile;
    std::string initFile;
    bool displayFeatures = true;
    bool opt_click_allowed = true;
    bool opt_display = true;
    bool cao3DModel = false;
    bool trackCylinder = true;
    bool useOgre = false;
    bool useScanline = false;
    bool quit = false;

    // Get the visp-images-data package path or VISP_INPUT_IMAGE_PATH environment variable value
    env_ipath = vpIoTools::getViSPImagesDataPath();

    // Set the default input path
    if (! env_ipath.empty())
      ipath = env_ipath;


    // Read the command line options
    if (!getOptions(argc, argv, opt_ipath, opt_configFile, opt_modelFile, opt_initFile, displayFeatures, opt_click_allowed, opt_display, cao3DModel, trackCylinder, useOgre, useScanline)) {
      return (-1);
    }

    // Test if an input path is set
    if (opt_ipath.empty() && env_ipath.empty() ){
      usage(argv[0], NULL);
      std::cerr << std::endl
                << "ERROR:" << std::endl;
      std::cerr << "  Use -i <visp image path> option or set VISP_INPUT_IMAGE_PATH "
                << std::endl
                << "  environment variable to specify the location of the " << std::endl
                << "  image path where test images are located." << std::endl
                << std::endl;

      return (-1);
    }

    // Get the option values
    if (!opt_ipath.empty())
      ipath = vpIoTools::createFilePath(opt_ipath, "ViSP-images/kltcylinder/img/%d.png");
    else
      ipath = vpIoTools::createFilePath(env_ipath, "ViSP-images/kltcylinder/img/%d.png");

    if (!opt_configFile.empty())
      configFile = opt_configFile;
    else if (!opt_ipath.empty())
      configFile = vpIoTools::createFilePath(opt_ipath, "ViSP-images/kltcylinder/cyl.xml");
    else
      configFile = vpIoTools::createFilePath(env_ipath, "ViSP-images/kltcylinder/cyl.xml");

    if (!opt_modelFile.empty()){
      modelFile = opt_modelFile;
    }else{
      std::string modelFileCao;
      std::string modelFileWrl;
      modelFileCao = "ViSP-images/kltcylinder/cyl.cao";
      modelFileWrl = "ViSP-images/kltcylinder/cyl.cao";

      if(!opt_ipath.empty()){
        modelFile = vpIoTools::createFilePath(opt_ipath, modelFileCao);
      }
      else{
        modelFile = vpIoTools::createFilePath(env_ipath, modelFileCao);
      }
    }

    if (!opt_initFile.empty())
      initFile = opt_initFile;
    else if (!opt_ipath.empty())
      initFile = vpIoTools::createFilePath(opt_ipath, "ViSP-images/kltcylinder/cyl");
    else
      initFile = vpIoTools::createFilePath(env_ipath, "ViSP-images/kltcylinder/cyl");

    vpImage<unsigned char> I;

    //    vpVideoReader reader;

    //    vp1394TwoGrabber reader(true); // Create a grabber based on libdc1394-2.x third party lib
    //    reader.setVideoMode(vp1394TwoGrabber::vpVIDEO_MODE_640x480_MONO8);
    //    reader.setFramerate(vp1394TwoGrabber::vpFRAMERATE_60);
    //    reader.open(I);


    //    reader.setFileName(ipath);
    //    try{
    //      reader.open(I);
    //    }catch(...){
    //      std::cout << "Cannot open sequence: " << ipath << std::endl;
    //      return -1;
    //    }

    //    reader.acquire(I);

    int opt_cam = 0;
    vpNaoqiGrabber g;
    g.setCamera(opt_cam); // left camera
    g.open();

    g.acquire(I);

    std::cout <<"Model path:" << modelFile << std::endl;

    // initialise a  display
#if defined VISP_HAVE_X11
    vpDisplayX display;
#elif defined VISP_HAVE_GDI
    vpDisplayGDI display;
#elif defined VISP_HAVE_OPENCV
    vpDisplayOpenCV display;
#elif defined VISP_HAVE_D3D9
    vpDisplayD3D display;
#elif defined VISP_HAVE_GTK
    vpDisplayGTK display;
#else
    opt_display = false;
#endif
    if (opt_display)
    {
#if (defined VISP_HAVE_DISPLAY)
      display.init(I, 100, 100, "Test tracking") ;
#endif
      vpDisplay::display(I) ;
      vpDisplay::flush(I);
    }

    //vpMbEdgeTracker tracker;
    //vpMbEdgeKltTracker tracker;
    vpMbKltTracker tracker;
    vpHomogeneousMatrix cMo;

    // Initialise the tracker: camera parameters, moving edge and KLT settings
    vpCameraParameters cam;
#if defined (VISP_HAVE_XML2)
    // From the xml file
    tracker.loadConfigFile(configFile);
#else
    // By setting the parameters:
    cam.initPersProjWithoutDistortion(547, 542, 338, 234);

    vpMe me;
    me.setMaskSize(5);
    me.setMaskNumber(180);
    me.setRange(7);
    me.setThreshold(5000);
    me.setMu1(0.5);
    me.setMu2(0.5);
    me.setSampleStep(4);

    tracker.setCameraParameters(cam);
    tracker.setMovingEdge(me);

    // Specify the clipping to use
    tracker.setNearClippingDistance(0.01);
    tracker.setFarClippingDistance(0.90);
    tracker.setClipping(tracker.getClipping() | vpMbtPolygon::FOV_CLIPPING);
    //   tracker.setClipping(tracker.getClipping() | vpMbtPolygon::LEFT_CLIPPING | vpMbtPolygon::RIGHT_CLIPPING | vpMbtPolygon::UP_CLIPPING | vpMbtPolygon::DOWN_CLIPPING); // Equivalent to FOV_CLIPPING
#endif

    // Display the moving edges, see documentation for the signification of the colours
    tracker.setDisplayFeatures(displayFeatures);

    // Tells if the tracker has to use Ogre3D for visibility tests
    tracker.setOgreVisibilityTest(useOgre);

    // Tells if the tracker has to use the scanline visibility tests
    tracker.setScanLineVisibilityTest(useScanline);

    cam = g.getCameraParameters();
    tracker.setCameraParameters(cam);

    //    // Retrieve the camera parameters from the tracker
    //    tracker.getCameraParameters(cam);

    // Loop to position the cube
    if (opt_display && opt_click_allowed)
    {
      while(!vpDisplay::getClick(I,false)){
        vpDisplay::display(I);
        vpDisplay::displayText(I, 15, 10, "click after positioning the object", vpColor::red);
        vpDisplay::flush(I) ;
      }
    }

    // Load the 3D model (either a vrml file or a .cao file)
    tracker.loadModel(modelFile);

    // Initialise the tracker by clicking on the image
    // This function looks for
    //   - a ./cube/cube.init file that defines the 3d coordinates (in meter, in the object basis) of the points used for the initialisation
    //   - a ./cube/cube.ppm file to display where the user have to click (optionnal, set by the third parameter)
    if (opt_display && opt_click_allowed)
    {
      tracker.initClick(I, initFile, true);
      tracker.getPose(cMo);
      // display the 3D model at the given pose
      tracker.display(I,cMo, cam, vpColor::red);
    }
    else
    {
      return 0;
    }

    //track the model
    tracker.track(I);
    tracker.getPose(cMo);

    if (opt_display)
      vpDisplay::flush(I);

    // Uncomment if you want to compute the covariance matrix.
    // tracker.setCovarianceComputation(true); //Important if you want tracker.getCovarianceMatrix() to work.

    while (true)
    {
      // acquire a new image
      g.acquire(I);
      // display the image
      if (opt_display)
        vpDisplay::display(I);

      tracker.track(I);
      tracker.getPose(cMo);
      if (opt_display) {
        // display the 3D model
        tracker.display(I, cMo, cam, vpColor::green,2,true);
        // display the frame
        vpDisplay::displayFrame (I, cMo, cam, 0.05);
      }

      if (opt_click_allowed) {
        vpDisplay::displayText(I, 10, 10, "Click to quit", vpColor::red);
        if (vpDisplay::getClick(I, false)) {
          quit = true;
          break;
        }
      }

      // Uncomment if you want to print the covariance matrix.
      // Make sure tracker.setCovarianceComputation(true) has been called (uncomment below).
      // std::cout << tracker.getCovarianceMatrix() << std::endl << std::endl;

      vpDisplay::flush(I) ;
    }

    if (opt_click_allowed && !quit) {
      vpDisplay::getClick(I);
    }


#if defined (VISP_HAVE_XML2)
    // Cleanup memory allocated by xml library used to parse the xml config file in vpMbEdgeTracker::loadConfigFile()
    vpXmlParser::cleanup();
#endif

#if defined(VISP_HAVE_COIN3D) && (COIN_MAJOR_VERSION == 3)
    // Cleanup memory allocated by Coin library used to load a vrml model in vpMbEdgeTracker::loadModel()
    // We clean only if Coin was used.
    if(! cao3DModel)
      SoDB::finish();
#endif

    return 0;
  }
  catch(vpException e) {
    std::cout << "Catch an exception: " << e << std::endl;
    return 1;
  }
}

#else

int main()
{
  std::cout << "visp_mbt module, Display is required to run this example." << std::endl;
  return 0;
}

#endif


/*
 * This example shows how instantiate an output window
 * 
 * How to compile:
 * c++ 00\ -\ Testing.cpp -std=c++17 -Wall -Wextra -lzuazo -lzuazo-window -lzuazo-ffmpeg -lavutil -lavformat -lavcodec -lglfw -ldl -lpthread
 */


#include <zuazo/Instance.h>
#include <zuazo/Modules/Window.h>
#include <zuazo/Modules/FFmpeg.h>
#include <zuazo/Consumers/Window.h>
#include <zuazo/Sources/FFmpegClip.h>

#include <mutex>
#include <iostream>

int main(int argc, const char** argv) {
	if(argc !=2 ) {
		std::cerr << "Usage: a.out <URL>" << std::endl;
		std::terminate();
	}

	//Instantiate Zuazo as usual. Note that FFmpeg module is loaded 
	Zuazo::Instance::ApplicationInfo appInfo (
		"FFmpeg Example 00",						//Application's name
		Zuazo::Version(0, 1, 0),					//Application's version
		Zuazo::Verbosity::GEQ_INFO,					//Verbosity 
		{ 	Zuazo::Modules::Window::get(), 			//Modules
			Zuazo::Modules::FFmpeg::get() }
	);
	Zuazo::Instance instance(std::move(appInfo));
	std::unique_lock<Zuazo::Instance> lock(instance);

	//Construct the desired video mode
	const Zuazo::VideoMode videoMode(
		Zuazo::Utils::MustBe<Zuazo::Rate>(Zuazo::Rate(25, 1)), //Just specify the desired rate
		Zuazo::Utils::Any<Zuazo::Resolution>(),
		Zuazo::Utils::Any<Zuazo::AspectRatio>(),
		Zuazo::Utils::Any<Zuazo::ColorPrimaries>(),
		Zuazo::Utils::Any<Zuazo::ColorModel>(),
		Zuazo::Utils::Any<Zuazo::ColorTransferFunction>(),
		Zuazo::Utils::Any<Zuazo::ColorSubsampling>(),
		Zuazo::Utils::Any<Zuazo::ColorRange>(),
		Zuazo::Utils::Any<Zuazo::ColorFormat>()	
	);

	//Construct the window object
	Zuazo::Consumers::Window window(
		instance, 						//Instance
		"Output Window",				//Layout name
		videoMode,						//Video mode limits
		Zuazo::Math::Vec2i(1280, 720),	//Window size (in screen coordinates)
		Zuazo::Consumers::Window::NO_MONITOR //No monitor
	);

	//Open the window (now becomes visible)
	window.open();



	Zuazo::Sources::FFmpegClip videoClip(instance, "Video Clip", Zuazo::VideoMode::ANY, std::string(argv[1]));
	videoClip.open();
	videoClip.setRepeat(Zuazo::ClipBase::Repeat::REPEAT);
	videoClip.play();
	//videoClip.setPlaySpeed(0.5);
	Zuazo::Signal::getInput<Zuazo::Video>(window) << Zuazo::Signal::getOutput<Zuazo::Video>(videoClip);

	//Done!
	lock.unlock();
	getchar();
	lock.lock();


	std::cout 	<< videoClip.getVideoModeCompatibility()[0].getFrameRate() << ", "
				<< videoClip.getVideoModeCompatibility()[0].getResolution() << ", "
				<< videoClip.getVideoModeCompatibility()[0].getPixelAspectRatio() << ", "
				<< videoClip.getVideoModeCompatibility()[0].getColorModel() << ", "
				<< videoClip.getVideoModeCompatibility()[0].getColorPrimaries() << ", "
				<< videoClip.getVideoModeCompatibility()[0].getColorTransferFunction() << ", "
				<< videoClip.getVideoModeCompatibility()[0].getColorRange() << ", "
				<< videoClip.getVideoModeCompatibility()[0].getColorSubsampling() << ", "
				<< videoClip.getVideoModeCompatibility()[0].getColorFormat() << std::endl;

}
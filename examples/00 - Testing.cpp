/*
 * This example shows how instantiate an output window
 * 
 * How to compile:
 * c++ 00\ -\ Testing.cpp -std=c++17 -Wall -Wextra -lzuazo -lzuazo-window -lzuazo-ffmpeg -lavutil -lavformat -lavcodec -lglfw -ldl -lpthread
 */


#include <zuazo/Instance.h>
#include <zuazo/Outputs/Window.h>
#include <zuazo/Inputs/FFmpegClip.h>

#include <mutex>
#include <iostream>

int main(int argc, const char** argv) {
	if(argc !=2 ) {
		std::cerr << "Usage: a.out <URL>" << std::endl;
		std::terminate();
	}

	//Load all the required window components. This *MUST* be done before instantiating Zuazo
	Zuazo::Outputs::Window::init();

	//Instantiate Zuazo as usual
	Zuazo::Instance::ApplicationInfo appInfo {
		"FFmpeg Example 00",						//Application's name
		Zuazo::Version(0, 1, 0),					//Application's version
		Zuazo::Verbosity::GEQ_INFO,					//Verbosity 
		Zuazo::VideoMode::ANY,						//Default video-mode
		Zuazo::Instance::defaultInstanceLogFunc,	//Instance log callback
		Zuazo::Instance::defaultElementLogFunc		//Element log callback
	};
	Zuazo::Instance instance(std::move(appInfo));
	std::unique_lock<Zuazo::Instance> lock(instance);

	//Generaly, you want to enable event polling
	Zuazo::Outputs::Window::enableRegularEventPolling(instance);

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
	Zuazo::Outputs::Window window(
		instance, 						//Instance
		"Output Window",				//Layout name
		videoMode,						//Video mode limits
		Zuazo::Math::Vec2i(1280, 720),	//Window size (in screen coordinates)
		Zuazo::Outputs::Window::NO_MONITOR //No monitor
	);

	//Open the window (now becomes visible)
	window.open();



	Zuazo::Inputs::FFmpegClip videoClip(instance, "Video Clip", Zuazo::VideoMode::ANY, std::string(argv[1]));
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
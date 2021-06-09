/*
 * This example shows how instantiate a video clip
 * 
 * How to compile:
 * c++ 00\ -\ Instantiating.cpp -std=c++17 -Wall -Wextra -lzuazo -lzuazo-window -lzuazo-ffmpeg -lzuazo-compositor -lglfw -ldl -lpthread -lavutil -lavformat -lavcodec -lswscale
 */

#include <zuazo/Instance.h>
#include <zuazo/Player.h>
#include <zuazo/Modules/Window.h>
#include <zuazo/Renderers/Window.h>
#include <zuazo/Layers/VideoSurface.h>
#include <zuazo/Sources/FFmpegClip.h>

#include <mutex>
#include <iostream>

int main(int argc, const char* argv[]) {
	if(argc != 2) {
		std::cerr << "Usage: " << *argv << " <video_file>" << std::endl;
		std::terminate();
	}

	//Instantiate Zuazo as usual. Note that we're loading the Window module
	Zuazo::Instance::ApplicationInfo appInfo(
		"Window Example 00",						//Application's name
		Zuazo::Version(0, 1, 0),					//Application's version
		Zuazo::Verbosity::geqInfo,					//Verbosity 
		{ Zuazo::Modules::Window::get() }			//Modules
	);
	Zuazo::Instance instance(std::move(appInfo));
	std::unique_lock<Zuazo::Instance> lock(instance);

	//Construct the window object
	Zuazo::Renderers::Window window(
		instance, 						//Instance
		"Output Window",				//Layout name
		Zuazo::Math::Vec2i(1280, 720)	//Window size (in screen coordinates)
	);

	//Set the negotiation callback
	window.setVideoModeNegotiationCallback(Zuazo::DefaultVideoModeNegotiator(Zuazo::FrameRates::P30));

	//Open the window (now becomes visible)
	window.asyncOpen(lock);

	//Create a layer for rendering to the window
	Zuazo::Layers::VideoSurface videoSurface(
		instance,
		"Video Surface",
		window.getViewportSize()
	);

	window.setViewportSizeCallback(
		std::bind(
			&Zuazo::Layers::VideoSurface::setSize, 
			&videoSurface, 
			std::placeholders::_2
		)
	);

	window.setLayers({videoSurface});
	videoSurface.setScalingMode(Zuazo::ScalingMode::box);
	videoSurface.setScalingFilter(Zuazo::ScalingFilter::nearest);
	videoSurface.asyncOpen(lock);

	//Create a video source
	Zuazo::Sources::FFmpegClip videoClip(
		instance,
		"Video Source",
		std::string(argv[1])
	);

	videoClip.play();
	videoClip.setRepeat(Zuazo::ClipBase::Repeat::repeat);
	videoClip.asyncOpen(lock);

	//Create a player for playing the clip
	Zuazo::Player clipPlayer(instance, &videoClip);
	clipPlayer.enable();

	//Route the signal
	videoSurface << videoClip;

	//Done!
	lock.unlock();
	getchar();
	lock.lock();

	//Show the video mode
	std::cout << "\nSelected video-mode:\n";
	std::cout << "\t-" << videoClip.getVideoMode() << "\n";
}
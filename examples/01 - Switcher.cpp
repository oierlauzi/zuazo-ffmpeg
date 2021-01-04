/*
 * This example shows how to buil a simple video switcher
 * 
 * How to compile:
 * c++ 01\ -\ Switcher.cpp -std=c++17 -Wall -Wextra -lzuazo -lzuazo-window -lzuazo-ffmpeg -lzuazo-compositor -lglfw -ldl -lpthread -lavutil -lavformat -lavcodec -lswscale
 */


#include <zuazo/Instance.h>
#include <zuazo/Player.h>
#include <zuazo/Modules/Window.h>
#include <zuazo/Consumers/WindowRenderer.h>
#include <zuazo/Processors/Layers/VideoSurface.h>
#include <zuazo/Sources/FFmpegClip.h>

#include <mutex>
#include <iostream>


int main(int argc, const char** argv) {
	//Instantiate Zuazo as usual. Note that we're loading the Window module
	Zuazo::Instance::ApplicationInfo appInfo(
		"FFmpeg Example 01",						//Application's name
		Zuazo::Version(0, 1, 0),					//Application's version
		Zuazo::Verbosity::GEQ_INFO,					//Verbosity 
		{ Zuazo::Modules::Window::get() }			//Modules
	);
	Zuazo::Instance instance(std::move(appInfo));
	std::unique_lock<Zuazo::Instance> lock(instance);

	//Construct the desired parameters
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

	const Zuazo::Utils::Limit<Zuazo::DepthStencilFormat> depthStencil(
		Zuazo::Utils::MustBe<Zuazo::DepthStencilFormat>(Zuazo::DepthStencilFormat::NONE) //Not interested in the depth buffer
	);

	const auto windowSize = Zuazo::Math::Vec2i(1280, 720);

	const auto& monitor = Zuazo::Consumers::WindowRenderer::NO_MONITOR; //Not interested in the full-screen mode

	//Construct the window objects
	Zuazo::Consumers::WindowRenderer pgmWindow(
		instance, 						//Instance
		"Program Output Window",		//Layout name
		videoMode,						//Video mode limits
		depthStencil,					//Depth buffer limits
		windowSize,						//Window size (in screen coordinates)
		monitor							//Monitor for setting fullscreen
	);

	pgmWindow.setWindowName(pgmWindow.getName());
	pgmWindow.setResizeable(false); //Disable resizeing, as extra care needs to be taken
	pgmWindow.open();

	Zuazo::Consumers::WindowRenderer pvwWindow(
		instance, 						//Instance
		"Program Output Window",		//Layout name
		videoMode,						//Video mode limits
		depthStencil,					//Depth buffer limits
		windowSize,						//Window size (in screen coordinates)
		monitor							//Monitor for setting fullscreen
	);

	pvwWindow.setWindowName(pvwWindow.getName());
	pvwWindow.setResizeable(false); //Disable resizeing, as extra care needs to be taken
	pvwWindow.open();

	//Create a layer for each window
	Zuazo::Processors::Layers::VideoSurface pgmVideoSurface(
		instance,
		"Program Video Surface",
		&pgmWindow,
		pgmWindow.getVideoMode().getResolutionValue()
	);

	pgmWindow.setLayers({pgmVideoSurface});
	pgmVideoSurface.setScalingMode(Zuazo::ScalingMode::BOXED);
	pgmVideoSurface.setScalingFilter(Zuazo::ScalingFilter::CUBIC);

	Zuazo::Processors::Layers::VideoSurface pvwVideoSurface(
		instance,
		"Program Video Surface",
		&pvwWindow,
		pvwWindow.getVideoMode().getResolutionValue()
	);

	pvwWindow.setLayers({pvwVideoSurface});
	pvwVideoSurface.setScalingMode(Zuazo::ScalingMode::BOXED);
	pvwVideoSurface.setScalingFilter(Zuazo::ScalingFilter::CUBIC);

	//Create a player for each window
	Zuazo::Player pgmPlayer(instance);
	pgmPlayer.enable();
	Zuazo::Player pvwPlayer(instance);
	pvwPlayer.enable();

	//Create a FFmpegClip object array
	std::vector<Zuazo::Sources::FFmpegClip> clips;
	clips.reserve(argc - 1);
	for(size_t i = 1; i < static_cast<size_t>(argc); i++) {
		clips.emplace_back(
			instance, 								//Instance
			"Video Clip " + Zuazo::toString(i),		//Input name
			Zuazo::VideoMode::ANY,					//Don't care about the videomode (it is provided by the actual file)
			std::string(argv[i])					//URL to the file
		);
	}


	//Configure the callbacks
	const auto keyCallback = [&pgmVideoSurface, &pgmPlayer, &pvwVideoSurface, &pvwPlayer,  &clips] (Zuazo::Consumers::WindowRenderer&, 
																									Zuazo::KeyboardKey key, 
																									Zuazo::KeyEvent event, 
																									Zuazo::KeyModifiers)
	{
		auto& pgmIn = pgmVideoSurface.getInput();
		auto& pvwIn = pvwVideoSurface.getInput();

		if(event == Zuazo::KeyEvent::PRESS) {
			//We only care for presses
			switch(key) {
			case Zuazo::KeyboardKey::ENTER:
				{
					auto* auxSrc = pgmIn.getSource();
					auto* auxClip = pgmPlayer.getClip();

					pgmIn.setSource(pvwIn.getSource());
					pgmPlayer.setClip(pvwPlayer.getClip());
					pvwIn.setSource(auxSrc);
					pvwPlayer.setClip(auxClip);
				}
				break;


			case Zuazo::KeyboardKey::F1:
			case Zuazo::KeyboardKey::F2:
			case Zuazo::KeyboardKey::F3:
			case Zuazo::KeyboardKey::F4:
			case Zuazo::KeyboardKey::F5:
			case Zuazo::KeyboardKey::F6:
			case Zuazo::KeyboardKey::F7:
			case Zuazo::KeyboardKey::F8:
			case Zuazo::KeyboardKey::F9:
			case Zuazo::KeyboardKey::F10:
				//Switch program
				{
					const auto index = static_cast<int>(key) - static_cast<int>(Zuazo::KeyboardKey::F1);
					if(Zuazo::Math::isInRange(index, 0, static_cast<int>(clips.size() - 1))) {
						pgmIn << clips[index].getOutput();
						if(pvwPlayer.getClip() != &clips[index]) {
							//It should be active in only one player
							pgmPlayer.setClip(&clips[index]);
						}
					} else {
						pgmIn << Zuazo::Signal::noSignal;
						pgmPlayer.setClip(nullptr);
					}
				}
				break;


			case Zuazo::KeyboardKey::NB0:
			case Zuazo::KeyboardKey::NB1:
			case Zuazo::KeyboardKey::NB2:
			case Zuazo::KeyboardKey::NB3:
			case Zuazo::KeyboardKey::NB4:
			case Zuazo::KeyboardKey::NB5:
			case Zuazo::KeyboardKey::NB6:
			case Zuazo::KeyboardKey::NB7:
			case Zuazo::KeyboardKey::NB8:
			case Zuazo::KeyboardKey::NB9:
				//Switch preview
				{
					const auto index = static_cast<int>(key) - static_cast<int>(Zuazo::KeyboardKey::NB1);
					if(Zuazo::Math::isInRange(index, 0, static_cast<int>(clips.size() - 1))) {
						pvwIn << clips[index].getOutput();
						if(pgmPlayer.getClip() != &clips[index]) {
							//It should be active in only one player
							pvwPlayer.setClip(&clips[index]);
						}
					} else {
						pvwIn << Zuazo::Signal::noSignal;
						pvwPlayer.setClip(nullptr);
					}
				}
				break;

			default:
				//Unknown action
				break;
			}

			
		}
	};

	pgmWindow.setKeyboardCallback(keyCallback);
	pvwWindow.setKeyboardCallback(keyCallback);
	
	pgmVideoSurface.open();
	pvwVideoSurface.open();

	//Open the video files and, them into repeat mode and playing
	for(auto& clip : clips) {
		clip.open();
		clip.setRepeat(Zuazo::ClipBase::Repeat::REPEAT);
		clip.play();
	}

	//Done!
	lock.unlock();
	getchar();
	lock.lock();
}
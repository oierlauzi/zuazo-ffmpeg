/*
 * This example shows how to buil a simple video switcher
 * 
 * How to compile:
 * c++ 01\ -\ Switcher.cpp -std=c++17 -Wall -Wextra -lzuazo -lzuazo-window -lzuazo-ffmpeg -lavutil -lavformat -lavcodec -lglfw -ldl -lpthread
 */


#include <zuazo/Instance.h>
#include <zuazo/Outputs/Window.h>
#include <zuazo/Inputs/FFmpegClip.h>

#include <mutex>
#include <iostream>

template<typename T>
void cut(Zuazo::Signal::Layout::PadProxy<Zuazo::Signal::Input<T>>& dst1, Zuazo::Signal::Layout::PadProxy<Zuazo::Signal::Input<T>>& dst2) {
	//Query the sources
	auto* src1 = dst1.getSource();
	auto* src2 = dst2.getSource();

	//Swap them
	std::swap(src1, src2);

	//Write the new sources
	dst1.setSource(src1);
	dst2.setSource(src2);
}



int main(int argc, const char** argv) {
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


	//Construct the window objects
	Zuazo::Outputs::Window pgmWindow(
		instance, 						//Instance
		"Program Output Window",		//Layout name
		videoMode,						//Video mode limits
		Zuazo::Math::Vec2i(1280, 720),	//Window size (in screen coordinates)
		Zuazo::Outputs::Window::NO_MONITOR //No monitor
	);
	pgmWindow.setWindowName("Program"); //The actual displayed name

	Zuazo::Outputs::Window pvwWindow(
		instance, 						//Instance
		"Preview Output Window",		//Layout name
		videoMode,						//Video mode limits
		Zuazo::Math::Vec2i(1280, 720),	//Window size (in screen coordinates)
		Zuazo::Outputs::Window::NO_MONITOR //No monitor
	);
	pvwWindow.setWindowName("Preview");


	//Create a FFmpegClip object array
	std::vector<Zuazo::Inputs::FFmpegClip> clips;
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
	const auto keyCallback = [&pgmWindow, &pvwWindow, &clips] (	Zuazo::Outputs::Window&, 
																Zuazo::Outputs::Window::KeyboardKey key, 
																Zuazo::Outputs::Window::KeyboardEvent event, 
																Zuazo::Outputs::Window::KeyboardModifiers)
	{
		if(event == Zuazo::Outputs::Window::KeyboardEvent::PRESS) {
			//We only care for presses
			switch(key) {
			case Zuazo::Outputs::Window::KeyboardKey::ENTER:
				//Cut
				cut(Zuazo::Signal::getInput<Zuazo::Video>(pgmWindow), Zuazo::Signal::getInput<Zuazo::Video>(pvwWindow));
				break;

			case Zuazo::Outputs::Window::KeyboardKey::F1:
			case Zuazo::Outputs::Window::KeyboardKey::F2:
			case Zuazo::Outputs::Window::KeyboardKey::F3:
			case Zuazo::Outputs::Window::KeyboardKey::F4:
			case Zuazo::Outputs::Window::KeyboardKey::F5:
			case Zuazo::Outputs::Window::KeyboardKey::F6:
			case Zuazo::Outputs::Window::KeyboardKey::F7:
			case Zuazo::Outputs::Window::KeyboardKey::F8:
			case Zuazo::Outputs::Window::KeyboardKey::F9:
			case Zuazo::Outputs::Window::KeyboardKey::F10:
				//Switch program
				{
					const auto index = static_cast<int>(key) - static_cast<int>(Zuazo::Outputs::Window::KeyboardKey::F1);
					if(Zuazo::Math::isInRange(index, 0, static_cast<int>(clips.size() - 1))) {
						Zuazo::Signal::getInput<Zuazo::Video>(pgmWindow) << Zuazo::Signal::getOutput<Zuazo::Video>(clips[index]);
					} else {
						Zuazo::Signal::getInput<Zuazo::Video>(pgmWindow) << Zuazo::Signal::noSignal;
					}
				}
				break;


			case Zuazo::Outputs::Window::KeyboardKey::NB0:
			case Zuazo::Outputs::Window::KeyboardKey::NB1:
			case Zuazo::Outputs::Window::KeyboardKey::NB2:
			case Zuazo::Outputs::Window::KeyboardKey::NB3:
			case Zuazo::Outputs::Window::KeyboardKey::NB4:
			case Zuazo::Outputs::Window::KeyboardKey::NB5:
			case Zuazo::Outputs::Window::KeyboardKey::NB6:
			case Zuazo::Outputs::Window::KeyboardKey::NB7:
			case Zuazo::Outputs::Window::KeyboardKey::NB8:
			case Zuazo::Outputs::Window::KeyboardKey::NB9:
				//Switch preview
				{
					const auto index = static_cast<int>(key) - static_cast<int>(Zuazo::Outputs::Window::KeyboardKey::NB1);
					if(Zuazo::Math::isInRange(index, 0, static_cast<int>(clips.size() - 1))) {
						Zuazo::Signal::getInput<Zuazo::Video>(pvwWindow) << Zuazo::Signal::getOutput<Zuazo::Video>(clips[index]);
					} else {
						Zuazo::Signal::getInput<Zuazo::Video>(pvwWindow) << Zuazo::Signal::noSignal;
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

	//Open the windows (now becomes visible)
	pgmWindow.open();
	pvwWindow.open();

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
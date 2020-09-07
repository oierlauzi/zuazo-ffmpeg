/*
 * This example shows how instantiate an output window
 * 
 * How to compile:
 * c++ 00\ -\ Testing.cpp -std=c++17 -Wall -Wextra -lzuazo -lzuazo-window -lzuazo-ffmpeg -lavutil -lavformat -lavcodec -lglfw -ldl -lpthread
 */


#include <zuazo/Instance.h>
#include <zuazo/Outputs/Window.h>
#include <zuazo/Inputs/FFmpegDemuxer.h>
#include <zuazo/Processors/FFmpegDecoder.h>
#include <zuazo/Processors/FFmpegUploader.h>
#include <zuazo/FFmpeg/Signals.h>

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



	Zuazo::Inputs::FFmpegDemuxer demuxer(instance, "Demuxer", std::string(argv[1]));
	demuxer.open();
	const auto videoIndex = demuxer.findBestStream(Zuazo::FFmpeg::MediaType::VIDEO);
	const auto& codecParameters = demuxer.getStreams()[videoIndex].getCodecParameters();

	Zuazo::Processors::FFmpegDecoder decoder(instance, "Decoder", std::move(codecParameters));
	decoder.open();

	Zuazo::Processors::FFmpegUploader uploader(instance, "Uploader");
	uploader.open();

	Zuazo::Signal::getInput<Zuazo::FFmpeg::PacketStream>(decoder) << Zuazo::Signal::getOutput<Zuazo::FFmpeg::PacketStream>(demuxer, Zuazo::Signal::makeOutputName<Zuazo::FFmpeg::PacketStream>(videoIndex));
	Zuazo::Signal::getInput<Zuazo::FFmpeg::Video>(uploader) << Zuazo::Signal::getOutput<Zuazo::FFmpeg::Video>(decoder);
	Zuazo::Signal::getInput<Zuazo::Video>(window) << Zuazo::Signal::getOutput<Zuazo::Video>(uploader);

	const auto decodeCallback = std::make_shared<Zuazo::Instance::ScheduledCallback>(
		[&demuxer, &decoder, &videoIndex] {
			do {
				demuxer.update();
			} while(demuxer.getLastStreamIndex() != videoIndex);

			decoder.update();
		}
	);

	instance.addRegularCallback(decodeCallback, Zuazo::Instance::INPUT_PRIORITY);

	//Done!
	lock.unlock();
	getchar();
	lock.lock();

	instance.removeRegularCallback(decodeCallback);
}
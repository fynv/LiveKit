#define USE_WASAPI 1

#include "AudioIO.h"
#include "AudioPort.h"
#if USE_WASAPI
#include "AudioIOWASAPI.h"
#else
#include "AudioIOMME.h"
#endif
#include <unordered_map>

namespace LiveKit
{
#if USE_WASAPI
	typedef AudioOut::ImplWASAPI ChoosenAudioOut;
	typedef AudioIn::ImplWASAPI ChoosenAudioIn;
#else
	typedef AudioOut::ImplMME ChoosenAudioOut;
	typedef AudioIn::ImplMME ChoosenAudioIn;
#endif

	std::unordered_map<int, std::unique_ptr<AudioPort>> g_audio_ports;

	inline AudioPort* get_port(int port_id)
	{
		auto iter = g_audio_ports.find(port_id);
		if (iter == g_audio_ports.end())
		{
			g_audio_ports[port_id] = (std::unique_ptr<AudioPort>)(new AudioPort);
		}
		return g_audio_ports[port_id].get();
	}

	inline void erase_port(int port_id)
	{
		auto iter = g_audio_ports.find(port_id);
		if (iter != g_audio_ports.end())
		{
			g_audio_ports.erase(iter);
		}
	}

	class AudioOut::ImplPort : public Implementer
	{
	public:
		ImplPort(int audio_device_id, int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
		{
			m_port_id = audio_device_id;
			AudioPort* port = get_port(m_port_id);
			m_writer_id = port->AddWriter(samplerate, callback, eof_callback, user_ptr);
		}

		~ImplPort()
		{
			AudioPort* port = get_port(m_port_id);
			port->RemoveWriter(m_writer_id);
			if (port->IsIdling())
				erase_port(m_port_id);
		}

	private:
		int m_port_id;
		int m_writer_id;

	};

	const std::vector<std::string>& AudioOut::s_get_list_audio_devices(int* id_default)
	{
		return ChoosenAudioOut::s_get_list_audio_devices(id_default);
	}

	AudioOut::AudioOut(int audio_device_id, int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
	{
		if (audio_device_id >= 0)
		{
			m_impl = (std::unique_ptr<Implementer>)(new ChoosenAudioOut(audio_device_id, samplerate, callback, eof_callback, user_ptr));
		}
		else
		{
			m_impl = (std::unique_ptr<Implementer>)(new ImplPort(audio_device_id, samplerate, callback, eof_callback, user_ptr));
		}

	}

	AudioOut::~AudioOut()
	{

	}

	class AudioIn::ImplPort : public Implementer
	{
	public:
		ImplPort(int audio_device_id, int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
		{
			m_port_id = audio_device_id;
			AudioPort* port = get_port(m_port_id);
			m_reader_id = port->AddReader(samplerate, callback, eof_callback, user_ptr);
		}

		~ImplPort()
		{
			AudioPort* port = get_port(m_port_id);
			port->RemoveReader(m_reader_id);
			if (port->IsIdling())
				erase_port(m_port_id);
		}

	private:
		int m_port_id;
		int m_reader_id;
	};

	const std::vector<std::string>& AudioIn::s_get_list_audio_devices(int* id_default)
	{
		return ChoosenAudioIn::s_get_list_audio_devices(id_default);
	}

	AudioIn::AudioIn(int audio_device_id, int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
	{
		if (audio_device_id >= 0)
		{
			m_impl = (std::unique_ptr<Implementer>)(new ChoosenAudioIn(audio_device_id, samplerate, callback, eof_callback, user_ptr));
		}
		else
		{
			m_impl = (std::unique_ptr<Implementer>)(new ImplPort(audio_device_id, samplerate, callback, eof_callback, user_ptr));
		}
	}

	AudioIn::~AudioIn()
	{

	}

}


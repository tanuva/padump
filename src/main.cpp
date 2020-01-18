#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/mainloop.h>
#include <pulse/stream.h>

#include <iostream>
#include <fstream>
#include <string>

std::ofstream file;

void pa_stream_notify_cb(pa_stream *stream, void* /*userdata*/)
{
    const pa_stream_state state = pa_stream_get_state(stream);
    switch (state) {
    case PA_STREAM_FAILED:
	std::cout << "Stream failed\n";
	break;
    case PA_STREAM_READY:
	std::cout << "Stream ready\n";
	break;
    default:
	std::cout << "Stream state: " << state << std::endl;
    }
}

void pa_stream_read_cb(pa_stream *stream, const size_t /*nbytes*/, void* /*userdata*/)
{
    // Careful when to pa_stream_peek() and pa_stream_drop()!
    // c.f. https://www.freedesktop.org/software/pulseaudio/doxygen/stream_8h.html#ac2838c449cde56e169224d7fe3d00824
    int16_t *data = nullptr;
    size_t actualbytes = 0;
    if (pa_stream_peek(stream, (const void**)&data, &actualbytes) != 0) {
	std::cerr << "Failed to peek at stream data\n";
	return;
    }

    if (data == nullptr && actualbytes == 0) {
	// No data in the buffer, ignore.
	return;
    } else if (data == nullptr && actualbytes > 0) {
	// Hole in the buffer. We must drop it.
	if (pa_stream_drop(stream) != 0) {
	    std::cerr << "Failed to drop a hole! (Sounds weird, doesn't it?)\n";
	    return;
	}
    }

    // process data
    std::cout << ">> " << actualbytes << " bytes\n";
    file.write((const char*)data, actualbytes);
    file.flush();

    if (pa_stream_drop(stream) != 0) {
	std::cerr << "Failed to drop data after peeking.\n";
    }
}

void pa_server_info_cb(pa_context *ctx, const pa_server_info *info, void* /*userdata*/)
{
    std::cout << "Default sink: " << info->default_sink_name << std::endl;

    pa_sample_spec spec;
    spec.format = PA_SAMPLE_S16LE;
    spec.rate = 44100;
    spec.channels = 1;
    // Use pa_stream_new_with_proplist instead?
    pa_stream *stream = pa_stream_new(ctx, "output monitor", &spec, nullptr);

    pa_stream_set_state_callback(stream, &pa_stream_notify_cb, nullptr /*userdata*/);
    pa_stream_set_read_callback(stream, &pa_stream_read_cb, nullptr /*userdata*/);

    std::string monitor_name(info->default_sink_name);
    monitor_name += ".monitor";
    if (pa_stream_connect_record(stream, monitor_name.c_str(), nullptr, PA_STREAM_NOFLAGS) != 0) {
	std::cerr << "connection fail\n";
	return;
    }

    std::cout << "Connected to " << monitor_name << std::endl;
}

void pa_context_notify_cb(pa_context *ctx, void* /*userdata*/)
{
    const pa_context_state state = pa_context_get_state(ctx);
    switch (state) {
    case PA_CONTEXT_READY:
	std::cout << "Context ready\n";
	pa_context_get_server_info(ctx, &pa_server_info_cb, nullptr /*userdata*/);
	break;
    case PA_CONTEXT_FAILED:
	std::cout << "Context failed\n";
	break;
    default:
	std::cout << "Context state: " << state << std::endl;
    }
}

int main(int argc, char **argv)
{
    file.open("out.pcm", std::ios::binary | std::ios::trunc);

    pa_mainloop *loop = pa_mainloop_new();
    pa_mainloop_api *api = pa_mainloop_get_api(loop);
    pa_context *ctx = pa_context_new(api, "padump");
    pa_context_set_state_callback(ctx, &pa_context_notify_cb, nullptr /*userdata*/);
    if (pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
	std::cerr << "PA connection failed.\n";
	return -1;
    }


    pa_mainloop_run(loop, nullptr);

    // pa_stream_disconnect(..)

    pa_context_disconnect(ctx);
    pa_mainloop_free(loop);
    file.close();
    return 0;
}

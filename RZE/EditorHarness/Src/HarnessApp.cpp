#include <HarnessApp.h>

#include <Windowing/Window.h>

#include <RapidJSON/document.h>
#include <RapidJSON/stringbuffer.h>
#include <RapidJSON/writer.h>

namespace EditorHarness
{
	namespace
	{
		using ReplyWriter = rapidjson::Writer<rapidjson::StringBuffer>;

		void Fail(ReplyWriter& reply, const std::string& error)
		{
			reply.Key("ok");
			reply.Bool(false);
			reply.Key("error");
			reply.String(error.c_str());
		}

		void Succeed(ReplyWriter& reply)
		{
			reply.Key("ok");
			reply.Bool(true);
		}
	}

	void HarnessApp::PreUIUpdate()
	{
		EditorApp::PreUIUpdate();

		++m_frameCount;
		if (m_frameCount == 1)
		{
			// The driver waits for this before sending commands
			m_channel.Reply(R"({"event":"ready"})");
		}

		for (const std::string& line : m_channel.TakeCommands())
		{
			m_channel.Reply(RunCommand(line));
		}
	}

	std::string HarnessApp::RunCommand(const std::string& line)
	{
		rapidjson::StringBuffer buffer;
		ReplyWriter reply(buffer);
		reply.StartObject();

		rapidjson::Document command;
		command.Parse(line.c_str());
		if (command.HasParseError() || !command.IsObject() || !command.HasMember("cmd") || !command["cmd"].IsString())
		{
			Fail(reply, R"(Not a command: expected {"cmd": "...", "id": n})");
			reply.EndObject();
			return buffer.GetString();
		}

		// Echoed, so the driver can match the answer to its command
		if (command.HasMember("id") && command["id"].IsInt64())
		{
			reply.Key("id");
			reply.Int64(command["id"].GetInt64());
		}

		const std::string name = command["cmd"].GetString();
		Window& window = *GetWindow();
		if (name == "ping")
		{
			Succeed(reply);
		}
		else if (name == "info")
		{
			Succeed(reply);
			reply.Key("title");
			reply.String(window.GetTitle().c_str());
			reply.Key("width");
			reply.Int(static_cast<int>(window.GetClientSize().X()));
			reply.Key("height");
			reply.Int(static_cast<int>(window.GetClientSize().Y()));
			reply.Key("frame");
			reply.Uint64(m_frameCount);
		}
		else if (name == "quit")
		{
			// As the window's close button does: unsaved changes are asked about first
			window.RequestClose();
			Succeed(reply);
		}
		else
		{
			Fail(reply, "Unknown command: " + name);
		}

		reply.EndObject();
		return buffer.GetString();
	}
}

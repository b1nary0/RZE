#include <HarnessApp.h>

#include <Windowing/Window.h>

#include <RapidJSON/stringbuffer.h>
#include <RapidJSON/writer.h>

#include <functional>

namespace EditorHarness
{
	namespace
	{
		using ReplyWriter = rapidjson::Writer<rapidjson::StringBuffer>;
		using ReplyFields = std::function<void(ReplyWriter&)>;

		// No id for a command that didn't give one
		constexpr int64_t k_noID = -1;

		// One answer: ok, or the error, with the command's id echoed so the driver can match them
		std::string MakeReply(int64_t id, const std::string& error, const ReplyFields& fields = nullptr)
		{
			rapidjson::StringBuffer buffer;
			ReplyWriter reply(buffer);
			reply.StartObject();
			if (id != k_noID)
			{
				reply.Key("id");
				reply.Int64(id);
			}
			reply.Key("ok");
			reply.Bool(error.empty());
			if (!error.empty())
			{
				reply.Key("error");
				reply.String(error.c_str());
			}
			if (fields)
			{
				fields(reply);
			}
			reply.EndObject();
			return buffer.GetString();
		}

		bool GetNumber(const rapidjson::Value& command, const char* name, float& outValue)
		{
			if (!command.HasMember(name) || !command[name].IsNumber())
			{
				return false;
			}
			outValue = command[name].GetFloat();
			return true;
		}

		Uint8 ToSDLButton(const std::string& name)
		{
			if (name == "right")
			{
				return SDL_BUTTON_RIGHT;
			}
			if (name == "middle")
			{
				return SDL_BUTTON_MIDDLE;
			}
			return name == "left" ? SDL_BUTTON_LEFT : 0;
		}
	}

	void HarnessApp::PreUIUpdate()
	{
		EditorApp::PreUIUpdate();

		++m_frameCount;
		if (m_frameCount == 1)
		{
			m_input.Initialize(static_cast<SDL_Window*>(GetWindow()->GetNativeHandle()));
			// The driver waits for this before sending commands
			m_channel.Reply(R"({"event":"ready"})");
		}

		m_input.Tick();
		for (const std::string& line : m_channel.TakeCommands())
		{
			RunCommand(line);
		}
	}

	void HarnessApp::RunCommand(const std::string& line)
	{
		rapidjson::Document command;
		command.Parse(line.c_str());
		if (command.HasParseError() || !command.IsObject() || !command.HasMember("cmd") || !command["cmd"].IsString())
		{
			m_channel.Reply(MakeReply(k_noID, R"(Not a command: expected {"cmd": "...", "id": n})"));
			return;
		}

		const int64_t id = command.HasMember("id") && command["id"].IsInt64() ? command["id"].GetInt64() : k_noID;
		const std::string name = command["cmd"].GetString();
		Window& window = *GetWindow();
		if (name == "ping")
		{
			m_channel.Reply(MakeReply(id, ""));
		}
		else if (name == "info")
		{
			m_channel.Reply(MakeReply(id, "", [this, &window](ReplyWriter& reply)
				{
					reply.Key("title");
					reply.String(window.GetTitle().c_str());
					reply.Key("width");
					reply.Int(static_cast<int>(window.GetClientSize().X()));
					reply.Key("height");
					reply.Int(static_cast<int>(window.GetClientSize().Y()));
					reply.Key("frame");
					reply.Uint64(m_frameCount);
					reply.Key("realInputBlocked");
					reply.Bool(m_input.IsRealInputBlocked());
				}));
		}
		else if (name == "quit")
		{
			// As the window's close button does: unsaved changes are asked about first
			window.RequestClose();
			m_channel.Reply(MakeReply(id, ""));
		}
		else if (name == "realinput")
		{
			if (!command.HasMember("on") || !command["on"].IsBool())
			{
				m_channel.Reply(MakeReply(id, R"(realinput needs "on": true or false)"));
				return;
			}
			m_input.SetRealInputBlocked(!command["on"].GetBool());
			m_channel.Reply(MakeReply(id, ""));
		}
		else
		{
			std::string error;
			if (!PlayInput(name, command, id, error))
			{
				m_channel.Reply(MakeReply(id, error));
			}
		}
	}

	bool HarnessApp::PlayInput(const std::string& name, const rapidjson::Value& command, int64_t id, std::string& outError)
	{
		const Vector2D& client = GetWindow()->GetClientSize();
		const auto inside = [&client](float x, float y) { return x >= 0.0f && y >= 0.0f && x < client.X() && y < client.Y(); };
		const auto needPoint = [&command, &inside, &outError](const char* xName, const char* yName, float& x, float& y)
			{
				if (!GetNumber(command, xName, x) || !GetNumber(command, yName, y))
				{
					outError = std::string("Needs \"") + xName + "\" and \"" + yName + "\", in client-area pixels";
					return false;
				}
				if (!inside(x, y))
				{
					outError = "(" + std::to_string(static_cast<int>(x)) + ", " + std::to_string(static_cast<int>(y)) + ") is outside the client area";
					return false;
				}
				return true;
			};

		std::vector<HarnessInput::Step> steps;
		float x = 0.0f;
		float y = 0.0f;
		if (name == "move")
		{
			if (!needPoint("x", "y", x, y))
			{
				return false;
			}
			steps.push_back(m_input.Move(x, y));
		}
		else if (name == "click" || name == "dblclick")
		{
			if (!needPoint("x", "y", x, y))
			{
				return false;
			}
			const std::string buttonName = command.HasMember("button") && command["button"].IsString() ? command["button"].GetString() : "left";
			const Uint8 button = ToSDLButton(buttonName);
			if (button == 0)
			{
				outError = "\"button\" is left, right or middle";
				return false;
			}
			// A frame on each: ImGui finds what's under the mouse, then sees it pressed, then released
			steps.push_back(m_input.Move(x, y));
			const int clicks = name == "dblclick" ? 2 : 1;
			for (int click = 1; click <= clicks; ++click)
			{
				steps.push_back(m_input.Button(button, true, x, y, static_cast<Uint8>(click)));
				steps.push_back(m_input.Button(button, false, x, y, static_cast<Uint8>(click)));
			}
		}
		else if (name == "drag")
		{
			float x2 = 0.0f;
			float y2 = 0.0f;
			if (!needPoint("x", "y", x, y) || !needPoint("x2", "y2", x2, y2))
			{
				return false;
			}
			const int moves = 10;
			steps.push_back(m_input.Move(x, y));
			steps.push_back(m_input.Button(SDL_BUTTON_LEFT, true, x, y, 1));
			for (int move = 1; move <= moves; ++move)
			{
				const float t = static_cast<float>(move) / static_cast<float>(moves);
				steps.push_back(m_input.Move(x + (x2 - x) * t, y + (y2 - y) * t));
			}
			steps.push_back(m_input.Button(SDL_BUTTON_LEFT, false, x2, y2, 1));
		}
		else if (name == "scroll")
		{
			float notches = 0.0f;
			if (!needPoint("x", "y", x, y) || !GetNumber(command, "notches", notches))
			{
				if (outError.empty())
				{
					outError = R"(scroll needs "notches": + away from you, - towards)";
				}
				return false;
			}
			steps.push_back(m_input.Move(x, y));
			steps.push_back(m_input.Wheel(x, y, notches));
		}
		else if (name == "key")
		{
			if (!command.HasMember("keys") || !command["keys"].IsString())
			{
				outError = R"(key needs "keys", in SendKeys syntax, e.g. "^a45{ENTER}")";
				return false;
			}
			std::vector<KeyStroke> strokes;
			if (!ParseKeys(command["keys"].GetString(), strokes, outError))
			{
				return false;
			}
			for (const KeyStroke& stroke : strokes)
			{
				steps.push_back(m_input.Keys(stroke, true));
				steps.push_back(m_input.Keys(stroke, false));
			}
		}
		else
		{
			outError = "Unknown command: " + name;
			return false;
		}

		m_input.Play(std::move(steps), [this, id]() { m_channel.Reply(MakeReply(id, "")); });
		return true;
	}
}

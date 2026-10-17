#include "framework/EngineContext.h"
#include "script/ScriptRuntime.h"
#include <framework/reflection/Reflection.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <set>
#include <cctype>

namespace vega::script
{
	namespace
	{
		std::string Identifier(const std::string& name)
		{
			std::string result = "Vega_";
			for (unsigned char c : name) result += std::isalnum(c) ? static_cast<char>(c) : '_';
			return result;
		}
		const char* PropertyType(ly::reflect::PropertyKind kind)
		{
			using ly::reflect::PropertyKind;
			switch (kind)
			{
			case PropertyKind::Bool: return "boolean";
			case PropertyKind::String: return "string";
			case PropertyKind::Vector2: return "VegaVector";
			case PropertyKind::Color: return "table";
			case PropertyKind::ActorRef: return "ActorHandle|nil";
			case PropertyKind::List: return "table";
			case PropertyKind::Struct: return "table";
			default: return "number";
			}
		}
	}

	bool GenerateLuaTypes(const std::filesystem::path& output, std::string& error) noexcept { return GenerateLuaTypes(ly::EngineContext::Current(), output, error); }

	bool GenerateLuaTypes(ly::EngineContext& context, const std::filesystem::path& output, std::string& error) noexcept
	{
		try
		{
			std::ostringstream text;
			text << R"(---@meta
-- Generated from Vega's runtime TypeRegistry. Do not execute or edit this file.
---@class VegaVector
---@field x number
---@field y number
---@operator add(VegaVector):VegaVector
---@operator sub(VegaVector):VegaVector
---@operator mul(number):VegaVector
VegaVector = {}
---@return number
function VegaVector:length() end
---@return VegaVector
function VegaVector:normalize() end
---@param other VegaVector
---@return number
function VegaVector:dot(other) end
---@param x number
---@param y number
---@return VegaVector
function vec(x, y) end

---@class SoundHandle
SoundHandle = {}
function SoundHandle:stop() end
---@return boolean
function SoundHandle:is_playing() end
audio = {}
---@param path string Asset-relative path; must remain inside the asset root.
---@param volume? number 0..100, default 100.
---@param pitch? number Greater than 0 and at most 16, default 1.
---@return SoundHandle
function audio.play(path, volume, pitch) end
music = {}
---@param path string Asset-relative path; music loops until replaced or stopped.
---@param volume? number
---@param pitch? number
function music.play(path, volume, pitch) end
function music.stop() end
---@param path string
---@param seconds number 0..86400.
---@param volume? number
---@param pitch? number
function music.fade(path, seconds, volume, pitch) end

---@class TimerHandle
TimerHandle = {}
function TimerHandle:cancel() end
---@return boolean
function TimerHandle:is_active() end
timer = {}
---@param seconds number Nonnegative; fires on a subsequent script tick.
---@param fn fun(self:VegaScript)
---@return TimerHandle
function timer.after(seconds, fn) end
---@param seconds number Positive; at most one firing per tick.
---@param fn fun(self:VegaScript)
---@return TimerHandle
function timer.every(seconds, fn) end

---@class ComponentHandle
ComponentHandle = {}
---@param name VegaProperty
---@return any
function ComponentHandle:get(name) end
---@param name VegaProperty
---@param value boolean|number|string|VegaVector|{x:number,y:number}
---@return boolean success
---@return string error
function ComponentHandle:set(name, value) end
---@param name string Registered reflected function identifier; raises a Lua error on failure.
function ComponentHandle:call(name) end

---@class ActorHandle
ActorHandle = {}
---@return boolean
function ActorHandle:is_valid() end
---@return boolean
function ActorHandle:is_alive() end
---@return integer
function ActorHandle:id() end
---@return string
function ActorHandle:name() end
---@return string
function ActorHandle:type() end
---@return VegaVector
function ActorHandle:location() end
---@param x number
---@param y number
function ActorHandle:set_location(x, y) end
---@return number
function ActorHandle:rotation() end
---@param degrees number
function ActorHandle:set_rotation(degrees) end
---@return VegaVector
function ActorHandle:velocity() end
---@param x number
---@param y number
function ActorHandle:set_velocity(x, y) end
---@return VegaVector
function ActorHandle:forward() end
---@param x number
---@param y number
function ActorHandle:look_at(x, y) end
---@param other ActorHandle
---@return number
function ActorHandle:distance_to(other) end
function ActorHandle:destroy() end
---@return ActorHandle|nil
function ActorHandle:parent() end
---@return ActorHandle[]
function ActorHandle:children() end
---@param other ActorHandle
---@param keep_world? boolean Defaults to true.
---@return boolean
function ActorHandle:attach_to(other, keep_world) end
function ActorHandle:detach() end
---@param name VegaProperty
---@return any
function ActorHandle:get(name) end
---@param name VegaProperty
---@param value boolean|number|string|VegaVector|{x:number,y:number}
---@return boolean success
---@return string error
function ActorHandle:set(name, value) end
---@param name string Registered reflected function identifier; raises a Lua error on failure.
function ActorHandle:call(name) end
---@param type string
---@return ComponentHandle?
function ActorHandle:component(type) end
---@alias VegaMessage nil|boolean|number|string|table<string,VegaMessage>
---@param name string
---@param data? VegaMessage Copied; delivered on the recipient's next script tick.
---@return boolean queued
function ActorHandle:send(name, data) end

---@class VegaScript
---@field actor ActorHandle
---@field props table<string,boolean|number|string|VegaVector>
---@field properties? table<string,boolean|number|string|VegaVector>
---@field on_start? fun(self:VegaScript)
---@field on_create? fun(self:VegaScript) Legacy fallback for on_start.
---@field on_update? fun(self:VegaScript,dt:number)
---@field on_destroy? fun(self:VegaScript)
---@field on_overlap_begin? fun(self:VegaScript,other:ActorHandle)
---@field on_overlap_end? fun(self:VegaScript,other:ActorHandle)
---@field on_hit? fun(self:VegaScript,other:ActorHandle)
---@field on_message? fun(self:VegaScript,name:string,data:VegaMessage)
---@param self VegaScript
function on_start(self) end
---@param self VegaScript
function on_create(self) end
---@param self VegaScript
---@param dt number
function on_update(self, dt) end
---@param self VegaScript
function on_destroy(self) end
---@param self VegaScript
---@param other ActorHandle
function on_overlap_begin(self, other) end
---@param self VegaScript
---@param other ActorHandle
function on_overlap_end(self, other) end
---@param self VegaScript
---@param other ActorHandle
function on_hit(self, other) end
---@param self VegaScript
---@param name string
---@param data VegaMessage
function on_message(self, name, data) end

world = {}
---@param name string Scene entity name; includes pending spawns.
---@return ActorHandle?
function world.find(name) end
---@param type string Includes reflected subclasses.
---@return ActorHandle[]
function world.find_all_by_type(type) end
---@param x number
---@param y number
---@param radius number Queries the spatial index; matches actor origins.
---@param type? string
---@return ActorHandle[]
function world.find_in_radius(x, y, radius, type) end
---@return ActorHandle? Registered by the game, scoped to this world.
function world.player() end
---@param type_or_prefab_path string Spawnable type or asset-relative .prefab.json path.
---@param x number
---@param y number
---@param props? table<string,boolean|number|string|VegaVector>
---@return ActorHandle?
function world.spawn(type_or_prefab_path, x, y, props) end

input = {}
---@param key string A-Z, Space, Left, Right, Up, Down. Requires viewport input focus.
---@return boolean
function input.key_down(key) end
---@param key string
---@return boolean
function input.pressed(key) end
---@param key string
---@return boolean
function input.released(key) end
---@return VegaVector
function input.mouse_world() end
---@param ... any
function log(...) end
---@return number Wall time since instance creation.
function time() end

)";
			std::set<std::string> names;
			for (const auto* type : context.Types().RegisteredTypes())
			{
				const auto id = Identifier(type->name);
				const auto properties = type->AllProperties();
				text << "-- Reflected type: " << type->name << "\n---@alias " << id << "Property ";
				if (properties.empty()) text << "string";
				for (std::size_t index = 0; index < properties.size(); ++index)
				{
					const auto& name = properties[index]->name;
					names.insert(name);
					text << (index ? "|" : "") << nlohmann::json(name).dump();
				}
				text << "\n---@class " << id << ": " << (type->root == ly::reflect::TypeRoot::Actor ? "ActorHandle" : "ComponentHandle")
					<< "\n" << id << " = {}\n";
				for (const auto* property : properties)
					text << "---@overload fun(self:" << id << ",name:" << nlohmann::json(property->name).dump() << "):"
						<< PropertyType(property->kind) << "\n";
				text << "---@param name " << id << "Property\n---@return any\nfunction " << id << ":get(name) end\n";
				for (const auto* property : properties)
					if (property->setter)
						text << "---@overload fun(self:" << id << ",name:" << nlohmann::json(property->name).dump() << ",value:"
							<< PropertyType(property->kind) << "):boolean,string\n";
				text << "---@param name " << id << "Property\n---@param value any\n---@return boolean,string\nfunction " << id << ":set(name,value) end\n\n";
			}
			text << "---@alias VegaProperty ";
			bool first = true;
			for (const auto& name : names)
			{
				text << (first ? "" : "|") << nlohmann::json(name).dump();
				first = false;
			}
			if (first) text << "string";
			text << '\n';
			std::filesystem::create_directories(output.parent_path());
			std::ofstream file{ output, std::ios::binary | std::ios::trunc };
			file << text.str();
			file.close();
			if (!file) throw std::runtime_error("Cannot write Lua annotations: " + output.string());
			error.clear();
			return true;
		}
		catch (const std::exception& exception) { error = exception.what(); return false; }
		catch (...) { error = "Cannot generate Lua annotations"; return false; }
	}
}

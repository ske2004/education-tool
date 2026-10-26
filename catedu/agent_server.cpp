#include "agent_server.hpp"
#include "catedu/gui/editor/dispatcher.hpp"
#include "catedu/resources/resources.hpp"
#include "catedu/scene/gltf_export.hpp"
#include "catedu/scene/navigation.hpp"
#include "catedu/scene/world_file.hpp"
#include "catedu/sys/json_fields.hpp"
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#define dup _dup
#define dup2 _dup2
#define fdopen _fdopen
#define fileno _fileno
#else
#include <unistd.h>
#endif

// Paths can cross the whole town, which needs more search than the playtest's
// short NPC walks.
#define AGENT_PATH_ITERATIONS 50000
// Observations list at most this many of the nearest objects.
#define OBSERVED_OBJECTS_MAX 24

struct Agent
{
    std::string name;
    Vector2i pos;
    std::vector<Vector2i> path;
    size_t next;

    bool moving() const
    {
        return next < path.size();
    }
};

struct SpeechEvent
{
    int tick;
    size_t agent;
    std::string name;
    std::string text;
    Vector2i pos;
};

struct AgentServer
{
    Dispatcher world;
    ResourceSpec resources;
    std::vector<Agent> agents;
    std::vector<SpeechEvent> speech_events;
    int tick;
    FILE *out;
    bool quit;
};

static void reply(AgentServer &server, const std::string &json)
{
    fputs(json.c_str(), server.out);
    fputc('\n', server.out);
    fflush(server.out);
}

static void reply_error(AgentServer &server, const char *error)
{
    std::string json = "{\"ok\":false,\"error\":";
    append_json_string(json, error);
    json += '}';
    reply(server, json);
}

static void append_agent(std::string &json, const AgentServer &server,
                         size_t id)
{
    const Agent &agent = server.agents[id];
    appendf(json, "{\"agent\":%zu,\"name\":", id);
    append_json_string(json, agent.name.c_str());
    appendf(json, ",\"x\":%d,\"y\":%d,\"moving\":%s}", agent.pos.x, agent.pos.y,
            agent.moving() ? "true" : "false");
}

static void append_agents(std::string &json, const AgentServer &server)
{
    json += "\"agents\":[";
    for (size_t i = 0; i < server.agents.size(); i++)
    {
        if (i)
        {
            json += ',';
        }
        append_agent(json, server, i);
    }
    json += ']';
}

static void append_object(std::string &json, const Object &object)
{
    json += "{\"type\":";
    append_json_string(json, object_type_name(object.type));
    appendf(json, ",\"x\":%g,\"y\":%g,\"floors\":%d", object.x, object.y,
            object.floors);
    if (object.type == Object::Type::prop)
    {
        json += ",\"id\":";
        append_json_string(json, object.id);
    }
}

// Reads the agent a command is about, replying with an error if it's missing.
static Agent *command_agent(AgentServer &server, const char *line)
{
    double id;
    if (!json_match_double(line, "agent", &id) || id < 0 ||
        (size_t)id >= server.agents.size())
    {
        reply_error(server, "no such agent");
        return nullptr;
    }

    return &server.agents[(size_t)id];
}

static bool command_tile(AgentServer &server, const char *line, Vector2i *tile)
{
    double x, y;
    if (!json_match_double(line, "x", &x) || !json_match_double(line, "y", &y))
    {
        reply_error(server, "x and y are required");
        return false;
    }

    *tile = {(int32_t)floor(x), (int32_t)floor(y)};
    return true;
}

static Place *outdoors(AgentServer &server)
{
    return server.world.world.first;
}

static void handle_place(AgentServer &server, const char *line)
{
    char type_name[32] = {};
    Object object = {};
    if (!json_match_str(line, "type", type_name, sizeof type_name) ||
        !object_type_from_name(type_name, &object.type))
    {
        reply_error(server, "unknown object type");
        return;
    }

    double x, y, floors = 1;
    if (!json_match_double(line, "x", &x) || !json_match_double(line, "y", &y))
    {
        reply_error(server, "x and y are required");
        return;
    }
    json_match_double(line, "floors", &floors);
    if (floors < 1 || floors > 20)
    {
        reply_error(server, "floors must be between 1 and 20");
        return;
    }

    object.x = (float)x;
    object.y = (float)y;
    if (object.type == Object::Type::building ||
        object.type == Object::Type::castle)
    {
        object.floors = (int)floors;
    }
    else
    {
        object.floors = (int)floors;
    }
    if (object.type == Object::Type::prop)
    {
        json_match_str(line, "id", object.id, sizeof object.id);
    }

    if (!outdoors(server)->place_object(object))
    {
        reply_error(server, "the tiles are taken");
        return;
    }

    reply(server, "{\"ok\":true}");
}

static void handle_objects(AgentServer &server)
{
    std::string json = "{\"ok\":true,\"objects\":[";
    bool first = true;
    for (auto &object : iter(outdoors(server)->objects))
    {
        json += first ? "" : ",";
        first = false;
        append_object(json, object);
        json += '}';
    }
    json += "]}";
    reply(server, json);
}

static void handle_spawn(AgentServer &server, const char *line)
{
    Agent agent = {};
    char name[64] = {};
    if (!json_match_str(line, "name", name, sizeof name) || !name[0])
    {
        reply_error(server, "name is required");
        return;
    }
    if (!command_tile(server, line, &agent.pos))
    {
        return;
    }
    if (!is_walkable(outdoors(server), agent.pos))
    {
        reply_error(server, "the tile isn't walkable");
        return;
    }

    agent.name = name;
    server.agents.push_back(agent);

    std::string json = "{\"ok\":true,";
    json += "\"agent\":";
    append_agent(json, server, server.agents.size() - 1);
    json += '}';
    reply(server, json);
}

static void handle_go(AgentServer &server, const char *line)
{
    Agent *agent = command_agent(server, line);
    Vector2i target;
    if (!agent || !command_tile(server, line, &target))
    {
        return;
    }
    if (!is_walkable(outdoors(server), target))
    {
        reply_error(server, "the tile isn't walkable");
        return;
    }

    std::vector<Vector2i> path;
    if (target != agent->pos)
    {
        path = find_path(outdoors(server), agent->pos, target,
                         AGENT_PATH_ITERATIONS);
        if (path.empty())
        {
            reply_error(server, "the tile can't be reached");
            return;
        }
    }

    agent->path = path;
    agent->next = 0;

    std::string json;
    appendf(json, "{\"ok\":true,\"steps\":%zu}", path.size());
    reply(server, json);
}

static void handle_stop(AgentServer &server, const char *line)
{
    if (Agent *agent = command_agent(server, line))
    {
        agent->path.clear();
        agent->next = 0;
        reply(server, "{\"ok\":true}");
    }
}

static void handle_say(AgentServer &server, const char *line)
{
    Agent *agent = command_agent(server, line);
    if (!agent)
    {
        return;
    }

    char text[512] = {};
    if (!json_match_str(line, "text", text, sizeof text))
    {
        reply_error(server, "text is required");
        return;
    }

    SpeechEvent ev = {};
    ev.tick = server.tick;
    ev.agent = (size_t)(agent - server.agents.data());
    ev.name = agent->name;
    ev.text = text;
    ev.pos = agent->pos;

    server.speech_events.push_back(ev);
    if (server.speech_events.size() > 500)
    {
        server.speech_events.erase(server.speech_events.begin(),
                                   server.speech_events.begin() + 100);
    }

    reply(server, "{\"ok\":true}");
}

static void handle_speech(AgentServer &server)
{
    std::string json = "{\"ok\":true,\"events\":[";
    for (size_t i = 0; i < server.speech_events.size(); i++)
    {
        if (i)
        {
            json += ',';
        }
        const SpeechEvent &ev = server.speech_events[i];
        appendf(json, "{\"tick\":%d,\"agent\":%zu,\"name\":", ev.tick, ev.agent);
        append_json_string(json, ev.name.c_str());
        json += ",\"text\":";
        append_json_string(json, ev.text.c_str());
        appendf(json, ",\"x\":%d,\"y\":%d}", ev.pos.x, ev.pos.y);
    }
    json += "]}";
    reply(server, json);
}

static void handle_step(AgentServer &server, const char *line)
{
    double ticks = 1;
    json_match_double(line, "ticks", &ticks);
    if (ticks < 1)
    {
        reply_error(server, "ticks must be at least 1");
        return;
    }

    for (int i = 0; i < (int)ticks; i++)
    {
        server.tick++;
        for (Agent &agent : server.agents)
        {
            if (agent.moving())
            {
                agent.pos = agent.path[agent.next++];
            }
        }
    }

    std::string json;
    appendf(json, "{\"ok\":true,\"tick\":%d,", server.tick);
    append_agents(json, server);
    json += '}';
    reply(server, json);
}

static float tile_distance(Vector2i a, float x, float y)
{
    return hypotf(x - a.x, y - a.y);
}

static void handle_observe(AgentServer &server, const char *line)
{
    Agent *agent = command_agent(server, line);
    if (!agent)
    {
        return;
    }

    double radius = 10;
    json_match_double(line, "radius", &radius);

    struct Seen
    {
        Object *object;
        float distance;
    };
    std::vector<Seen> seen;
    for (auto &object : iter(outdoors(server)->objects))
    {
        float distance = tile_distance(agent->pos, object.x, object.y);
        if (distance <= radius)
        {
            seen.push_back({&object, distance});
        }
    }
    std::stable_sort(
        seen.begin(), seen.end(),
        [](const Seen &a, const Seen &b) { return a.distance < b.distance; });
    if (seen.size() > OBSERVED_OBJECTS_MAX)
    {
        seen.resize(OBSERVED_OBJECTS_MAX);
    }

    std::string json;
    appendf(json, "{\"ok\":true,\"tick\":%d,\"self\":", server.tick);
    append_agent(json, server, agent - server.agents.data());

    json += ",\"objects\":[";
    for (size_t i = 0; i < seen.size(); i++)
    {
        json += i ? "," : "";
        append_object(json, *seen[i].object);
        appendf(json, ",\"distance\":%.1f}", seen[i].distance);
    }

    json += "],\"agents\":[";
    bool first = true;
    for (size_t i = 0; i < server.agents.size(); i++)
    {
        const Agent &other = server.agents[i];
        float distance = tile_distance(agent->pos, other.pos.x, other.pos.y);
        if (&other == agent || distance > radius)
        {
            continue;
        }

        json += first ? "" : ",";
        first = false;
        append_agent(json, server, i);
        json.pop_back();
        appendf(json, ",\"distance\":%.1f}", distance);
    }
    json += "],\"speech\":[";
    bool first_speech = true;
    for (const SpeechEvent &ev : server.speech_events)
    {
        float distance = tile_distance(agent->pos, ev.pos.x, ev.pos.y);
        if (distance <= radius)
        {
            json += first_speech ? "" : ",";
            first_speech = false;
            appendf(json, "{\"tick\":%d,\"agent\":%zu,\"name\":", ev.tick, ev.agent);
            append_json_string(json, ev.name.c_str());
            json += ",\"text\":";
            append_json_string(json, ev.text.c_str());
            appendf(json, ",\"distance\":%.1f}", distance);
        }
    }
    json += "]}";
    reply(server, json);
}

static void handle_command(AgentServer &server, const char *line)
{
    char cmd[32] = {};
    char path[512] = {};
    if (!json_match_str(line, "cmd", cmd, sizeof cmd))
    {
        reply_error(server, "cmd is required");
        return;
    }

    if (strcmp(cmd, "new") == 0)
    {
        server.world.destroy();
        server.world = Dispatcher::create();
        server.agents.clear();
        server.speech_events.clear();
        server.tick = 0;
        reply(server, "{\"ok\":true}");
    }
    else if (strcmp(cmd, "load") == 0 || strcmp(cmd, "save") == 0 ||
             strcmp(cmd, "export") == 0)
    {
        if (!json_match_str(line, "path", path, sizeof path) || !path[0])
        {
            reply_error(server, "path is required");
            return;
        }

        bool ok = false;
        if (strcmp(cmd, "load") == 0)
        {
            // Loading a missing file gives an empty world, so check first.
            if (FILE *f = fopen(path, "rb"))
            {
                fclose(f);
                server.world.destroy();
                server.world = WorldFile::load(path);
                server.agents.clear();
                server.speech_events.clear();
                server.tick = 0;
                ok = true;
            }
        }
        else if (strcmp(cmd, "save") == 0)
        {
            ok = WorldFile::save(path, server.world);
        }
        else
        {
            GenResources resources = {};
            resources.spec = &server.resources;
            ok = export_world_gltf(path, server.world.world, resources);
        }

        if (ok)
        {
            reply(server, "{\"ok\":true}");
        }
        else
        {
            reply_error(server, "couldn't access the file");
        }
    }
    else if (strcmp(cmd, "place") == 0)
    {
        handle_place(server, line);
    }
    else if (strcmp(cmd, "objects") == 0)
    {
        handle_objects(server);
    }
    else if (strcmp(cmd, "spawn") == 0)
    {
        handle_spawn(server, line);
    }
    else if (strcmp(cmd, "go") == 0)
    {
        handle_go(server, line);
    }
    else if (strcmp(cmd, "stop") == 0)
    {
        handle_stop(server, line);
    }
    else if (strcmp(cmd, "step") == 0)
    {
        handle_step(server, line);
    }
    else if (strcmp(cmd, "observe") == 0)
    {
        handle_observe(server, line);
    }
    else if (strcmp(cmd, "say") == 0)
    {
        handle_say(server, line);
    }
    else if (strcmp(cmd, "speech") == 0 || strcmp(cmd, "events") == 0)
    {
        handle_speech(server);
    }
    else if (strcmp(cmd, "agents") == 0)
    {
        std::string json;
        appendf(json, "{\"ok\":true,\"tick\":%d,", server.tick);
        append_agents(json, server);
        json += '}';
        reply(server, json);
    }
    else if (strcmp(cmd, "quit") == 0)
    {
        server.quit = true;
        reply(server, "{\"ok\":true}");
    }
    else
    {
        reply_error(server, "unknown command");
    }
}

int run_agent_server()
{
    AgentServer server = {};

    // Answers go to the original stdout. Anything else the engine prints goes
    // to stderr so it can't corrupt the protocol.
    server.out = fdopen(dup(fileno(stdout)), "w");
    if (!server.out)
    {
        return 1;
    }
    dup2(fileno(stderr), fileno(stdout));

    server.world = Dispatcher::create();
    server.resources = load_resource_names();

    std::string line;
    char chunk[4096];
    while (!server.quit && fgets(chunk, sizeof chunk, stdin))
    {
        line += chunk;
        if (line.empty() || line.back() != '\n')
        {
            continue;
        }

        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
        {
            line.pop_back();
        }
        if (!line.empty())
        {
            handle_command(server, line.c_str());
        }
        line.clear();
    }

    server.world.destroy();
    server.resources.models.deinit();
    fclose(server.out);
    return 0;
}

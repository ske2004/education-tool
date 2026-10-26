#include "navigation.hpp"
#include <algorithm>
#include <queue>
#include <unordered_map>

namespace {

struct AStarNode {
    Vector2i pos;
    float g_cost;
    float f_cost;
    
    bool operator>(const AStarNode& other) const {
        return f_cost > other.f_cost;
    }
};

struct Vector2iHash {
    size_t operator()(const Vector2i& v) const {
        return std::hash<int32_t>()(v.x) ^ (std::hash<int32_t>()(v.y) << 1);
    }
};

} // namespace

bool is_walkable(Place *place, Vector2i pos) {
    Object *obj = place->get_object_at(pos.x, pos.y);
    if (!obj) return true;
    if (obj->type == Object::Type::building || 
        obj->type == Object::Type::castle ||
        obj->type == Object::Type::wall || 
        obj->type == Object::Type::tree ||
        obj->type == Object::Type::water) {
        return false;
    }
    return true; 
}

std::vector<Vector2i> find_path(Place *place, Vector2i start, Vector2i end,
                               int max_iterations) {
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
    std::unordered_map<Vector2i, Vector2i, Vector2iHash> came_from;
    std::unordered_map<Vector2i, float, Vector2iHash> g_score;
    
    open_set.push({start, 0.0f, vector2_length({(float)end.x - start.x, (float)end.y - start.y})});
    g_score[start] = 0.0f;
    
    int iterations = 0;
    while (!open_set.empty() && iterations < max_iterations) {
        AStarNode top = open_set.top();
        open_set.pop();
        Vector2i current = top.pos;

        auto it = g_score.find(current);
        if (it != g_score.end() && top.g_cost > it->second) {
            continue;
        }
        iterations++;
        
        if (current == end) {
            std::vector<Vector2i> path;
            while (current != start) {
                path.push_back(current);
                current = came_from[current];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        Vector2i neighbors[4] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
        for (auto n : neighbors) {
            Vector2i next = current + n;
            if (!is_walkable(place, next)) continue;
            
            float tentative_g = g_score[current] + 1.0f;
            if (g_score.find(next) == g_score.end() || tentative_g < g_score[next]) {
                came_from[next] = current;
                g_score[next] = tentative_g;
                float f = tentative_g + vector2_length({(float)end.x - next.x, (float)end.y - next.y});
                open_set.push({next, tentative_g, f});
            }
        }
    }
    return {};
}

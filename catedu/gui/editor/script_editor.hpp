#pragma once
#include "catedu/scene/script.hpp"
#include "catedu/scene/world.hpp"
#include "catedu/ui/user.hpp"

struct ScriptCardAction
{
    ScriptNode *parent;
    bool yes;
};

struct ScriptCardData
{
    bool dragging;
    Vector2 drag_offset;
    Vector2 drag_peg;
};

struct ScriptCardDragNDrop
{
    ScriptNode *node;
    ScriptNode *insert_after;
    ScriptNode *insert_after_final;
    ScriptCardData *dragging;
};

struct ScriptCardPalettePool
{
    ScriptNode *nodes[int(ScriptNode::Type::count_)];
};

struct ScriptEditor
{
    Script *script;
    ScriptNode *current;
    ScriptCardDragNDrop dnd;
    ScriptCardPalettePool palette;

    void show_palette(UiPass &user);
    void show(UiPass &user, World &world);
    static ScriptEditor create(Script *script);
};

#include "user.hpp"
#include "catedu/core/alloc/arena.hpp"
#include "catedu/core/storage/stack.hpp"
#include "catedu/ui/layout/dpi_scale.hpp"
#include "catedu/ui/rendering/transform.hpp"
#include "resources/load_image.hpp"

void draw_brush(UiRenderingPass &pass, Rect rect, UiBrush brush)
{
    UiTransform transform = {};
    transform.scale = {1, 1};
    transform.base = rect;

    pass.push_transform(transform);

    pass.render_brush(brush);

    pass.pop_transform();
}

UiPass UiPass::init(UiState &state)
{
    return UiPass{&state};
}

void UiPass::begin_pass()
{
    this->state->frame_storage.reset();
    this->layout = AutoLayoutProcess::init(this->state->frame_storage,
                                           &this->current_node);
    assert(this->current_node != nullptr && "Root node failed to initialize");
    this->state->element_storage.begin_cycle();
    if (this->state->dpi_scale > 10.0)
    {
        this->state->dpi_scale = 10.0;
    }
}

struct Renderable
{
    UiGenericStyles *styles;
    // These boxes are absolute.
    Rect border_box;
    Rect base_box;
    // These boxes are relative to the parent.
    Rect border_box_rel;
    Rect base_box_rel;
};

struct PoppedElement
{
    Vector2 suboffs;
    UiTransform transform;
    AutoLayoutResult *result;
};

static void render_object(Stack<PoppedElement> &popped, UiPass &user,
                          AutoLayoutResult &result, Vector2 suboffs = {0, 0})
{

    UiGenericStyles *styles = (UiGenericStyles *)result.userdata;

    Rect margin_box_rel = result.margin_box;
    margin_box_rel.pos -= suboffs;
    Rect padding_box_rel = result.padding_box;
    padding_box_rel.pos -= suboffs;
    Rect border_box_rel = result.border_box;
    border_box_rel.pos -= suboffs + margin_box_rel.pos;
    Rect base_box_rel = result.base_box;
    base_box_rel.pos -= suboffs + margin_box_rel.pos;

    UiTransform transform = {};
    transform.base = margin_box_rel;
    transform.origin = {0.5, 0.5};
    transform.scale = {1, 1};
    if (styles != nullptr)
    {
        transform.scale = {styles->scale, styles->scale};
    }
    user.pass.push_transform(transform);

    UiFilter prev_filter = user.state->core->filter;

    // TODO: Why would style be null?
    if (styles != nullptr)
    {
        if (styles->has_filter)
        {
            user.state->core->filter = styles->filter;
        }

        // FIXME: Way too into the guts of the system.
        auto *pe = user.state->element_storage.elements.get(styles->persistent);
        if (pe)
        {
            pe->border_box = result.border_box;
        }

        if (styles->text)
        {
            UiFontRenderer &font =
                styles->bold ? user.state->font_bold : user.state->font;

            font.render_text_utf8(&user.pass, base_box_rel.pos, styles->text,
                                  styles->brush,
                                  styles->text_scale * user.state->dpi_scale);
        }
        else
        {
            draw_brush(user.pass, border_box_rel, styles->border);
            draw_brush(user.pass, base_box_rel, styles->brush);
        }
    }

    if (result.clip)
    {
        user.pass.begin_scissor(result.padding_box);
    }

    AutoLayoutResult *child = result.child;
    while (child)
    {
        if (child->pop)
        {
            popped.push({margin_box_rel.pos,
                         user.pass.transformer.transforms.back(), child});
        }
        else
        {
            render_object(popped, user, *child, suboffs + margin_box_rel.pos);
        }
        child = child->sibling;
    }

    user.state->core->filter = prev_filter;

    if (result.clip)
    {
        user.pass.end_scissor();
    }

    user.pass.pop_transform();
}

static void render_out(UiPass &user)
{
    AutoLayoutResult *result;
    user.layout.process(user.state->frame_storage, result);
    dpi_rescale_autolayout_result(result, user.state->dpi_scale);

    if (result == nullptr)
    {
        return;
    }

    Stack<PoppedElement> popped = {};
    render_object(popped, user, *result);
    while (popped.count > 0)
    {
        Stack<PoppedElement> next_popped = {};
        for (auto &p : iter(popped))
        {
            user.pass.push_transform(p.transform);
            render_object(next_popped, user, *p.result, p.suboffs);
            user.pass.pop_transform();
        }
        popped.deinit();
        popped = next_popped;
    }

    assert(user.current_node == user.layout.root &&
           "Unfinished begin_generic calls");
}

void UiPass::end_pass()
{
    this->pass = UiRenderingPass::begin(state->core);
    render_out(*this);
    this->pass.end();

    UiInteractionStatePass interaction = {};
    interaction.interaction_table = &this->state->interaction_table;
    interaction.element_storage = &this->state->element_storage;
    interaction.mouse_pos = this->state->input.mouse_pos;
    interaction.interaction = this->state->input.k[INPUT_MB_LEFT].pressed;
    interaction.active_interaction = this->state->input.k[INPUT_MB_LEFT].held;
    interaction.exit_interaction =
        this->state->input.k[SAPP_KEYCODE_ESCAPE].pressed;
    interaction.switch_interaction =
        this->state->input.k[SAPP_KEYCODE_TAB].pressed;
    interaction.process();

    this->state->input.update();
    this->state->element_storage.end_cycle();
}

void UiPass::push_id(int64_t id)
{
    this->state->element_storage.push(std::to_string(id).c_str(), {});
}

void UiPass::push_id(const char *id)
{
    this->state->element_storage.push(id, {});
}

void UiPass::pop_id()
{
    this->state->element_storage.pop();
}

bool UiPass::focused()
{
    return state->interaction_table.focused == state->element_storage.id();
}

bool UiPass::active()
{
    return state->interaction_table.active == state->element_storage.id();
}

bool UiPass::actively_hovered()
{
    return state->interaction_table.hovered == state->element_storage.id();
}

bool UiPass::hovered()
{
    auto el = state->element_storage.value();
    return rect_vs_vector2(el->border_box, state->input.mouse_pos);
}

void UiPass::begin_generic(AutoLayoutElement el, UiBrush brush, UiBrush border,
                           TableId persistent_id, float scale, bool has_filter,
                           UiFilter filter)
{
    UiGenericStyles styles = {brush, border, nullptr,    {1, 1}, persistent_id,
                              false, scale,  has_filter, filter};

    el.userdata = this->state->frame_storage.alloct<UiGenericStyles>();
    *(UiGenericStyles *)el.userdata = styles;

    this->current_node = this->layout.add_element(this->current_node, el);
}

void UiPass::end_generic()
{
    assert(this->current_node && "Begin generic was not called");

    this->current_node = this->current_node->parent;
}

bool UiState::feed_event(const sapp_event *event)
{
    this->input.pass_event(event);
    if (this->input.shortcut(MOD_CTRL, SAPP_KEYCODE_MINUS))
    {
        this->dpi_scale /= 1.125;
        if (this->dpi_scale < 0.25f)
        {
            this->dpi_scale = 0.25f;
        }
    }
    if (this->input.shortcut(MOD_CTRL, SAPP_KEYCODE_EQUAL))
    {
        this->dpi_scale *= 1.125;
    }

    return this->interaction_table.focused != NULL_ID ||
           this->interaction_table.hovered != NULL_ID;
}

UiImageId UiState::get_image(const char *path)
{
    UiImageId id = NULL_ID;
    for (auto [i, img] : iter(image_cache))
    {
        if (strcmp(img.path, path) == 0)
        {
            id = img.id;
            break;
        }
    }

    if (id == NULL_ID)
    {
        id = ui_resources_load_image(core, path);

        char *newpath =
            strcpy((char *)lifetime_storage.alloc(strlen(path) + 1), path);

        image_cache.allocate({newpath, id});
    }

    return id;
}

UiState UiState::init(const char *font_path, const char *font_bold_path,
                      float dpi_scale)
{
    UiState state = {};
    state.dpi_scale = dpi_scale;
    state.core =
        (UiRenderingCore *)ALLOCATOR_MALLOC.alloc(sizeof(UiRenderingCore));
    *state.core = UiRenderingCore::init();
    state.font =
        UiFontRenderer::init(state.core, {font_path, 16}, dpi_scale * 2);
    state.font_bold =
        UiFontRenderer::init(state.core, {font_bold_path, 16}, dpi_scale * 2);
    state.frame_storage = Arena::create(&ALLOCATOR_MALLOC);
    state.lifetime_storage = Arena::create(&ALLOCATOR_MALLOC);
    return state;
}

void UiState::deinit()
{
    for (auto [id, el] : iter(this->image_cache))
    {
        this->core->dealloc_image(el.id);
    }

    this->image_cache.deinit();
    this->element_storage.deinit();
    this->font_bold.deinit();
    this->font.deinit();
    this->core->deinit();
    this->frame_storage.destroy();
    this->lifetime_storage.destroy();
    ALLOCATOR_MALLOC.free(this->core);
}

void UiElementStorage::begin_cycle()
{
    this->element_retainer.begin_cycle();
    this->order = 0;
}

void UiElementStorage::end_cycle()
{
    this->element_retainer.end_cycle();
}

void UiElementStorage::push(const char *id, UiPersistentElement value)
{
    if (this->element_retainer.has(id) == false)
    {
        this->element_retainer.push(id, this->elements.allocate(value));
    }
    else
    {
        // TODO: This is a hack to make sure that the element isn't reallocated.
        this->element_retainer.push(id, {});
    }
    this->value()->order = this->order;
    this->order++;
}

void UiElementStorage::pop()
{
    this->element_retainer.pop();
}

UiPersistentElement *UiElementStorage::value()
{
    return &this->elements.get_assert(this->element_retainer.value());
}

TableId UiElementStorage::id()
{
    return this->element_retainer.value();
}

UiElementStorage UiElementStorage::init()
{
    return UiElementStorage();
}

void UiElementStorage::deinit()
{
    for (auto [id, el] : iter(this->elements))
    {
        if (el.userdata)
        {
            ALLOCATOR_MALLOC.free(el.userdata);
        }
    }
    this->elements.deinit();
    this->element_retainer.deinit();
}

void UiInteractionStatePass::process()
{
    TableId hovered = NULL_ID;
    TableId focused = this->interaction_table->focused;
    TableId prev_focused = this->interaction_table->focused;

    if (this->exit_interaction || this->interaction || this->switch_interaction)
    {
        focused = NULL_ID;
    }

    int max_order = -1;

    for (auto [id, el] : iter(this->element_storage->elements))
    {
        if (this->switch_interaction && el.focusable)
        {
            if (prev_focused != NULL_ID)
            {
                UiPersistentElement &pe =
                    this->element_storage->elements.get_assert(prev_focused);
                if (el.order > pe.order)
                {
                    focused = id;
                    this->switch_interaction = false;
                }
            }
        }

        if (rect_vs_vector2(el.border_box, this->mouse_pos) &&
            (el.order > max_order || el.pop))
        {
            if (this->interaction && el.focusable)
            {
                focused = id;
            }

            max_order = el.order;
            hovered = id;
        }

        // TODO: This is a hack to avoid freeing elements in the table for now.
        //       This should be removed when the element storage is refactored.
        el.order = -1;
    }

    TableId active = NULL_ID;

    if (this->active_interaction)
    {
        if (this->interaction_table->active == NULL_ID)
        {
            active = this->interaction_table->hovered;
        }
        else
        {
            active = this->interaction_table->active;
        }
    }

    this->interaction_table->hovered = hovered;
    this->interaction_table->active = active;
    this->interaction_table->focused = focused;
}

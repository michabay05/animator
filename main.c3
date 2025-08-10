import std::io;
import std::io::file;
import std::collections::list;
import std::encoding::json;

import animator;
import rl;
import ffmpeg;
import jimp;

// convert jimp.h into jimp.c3

// struct Text
// {
//     char *text;
//     float font_size;
//     Vector2 position;
//     Color color;
// }

fn bool parse_str(Jimp *jimp, String *str) @local
{
    if (!jimp::string(jimp)) return false;
    *str = ((ZString)jimp.string).tcopy();
    return true;
}

fn bool parse_vec2(Jimp *jimp, Vector2 *v) @local
{
    if (!jimp::array_begin(jimp)) return false;
    int i = 0;
    while (jimp::array_item(jimp)) {
        if (i >= rl::Vector2.len) return false;
        if (!jimp::number(jimp)) return false;
        (*v)[i++] = (float)jimp.number;
    }
    return jimp::array_end(jimp);
}

fn bool parse_color(Jimp *jimp, Color *c) @local
{
    if (!jimp::array_begin(jimp)) return false;
    int i = 0;
    while (jimp::array_item(jimp)) {
        if (i >= rl::Color.len) return false;
        if (!jimp::number(jimp)) return false;
        (*c)[i++] = (char)jimp.number;
    }
    return jimp::array_end(jimp);
}

fn bool parse_text(Jimp *jimp, Text *t) @local
{
    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "text") {
            if (!parse_str(jimp, &t.text)) return false;
        } else if (member == "fontSize") {
            if (!jimp::number(jimp)) return false;
            t.font_size = (float)jimp.number;
        } else if (member == "position") {
            if (!parse_vec2(jimp, &t.position)) return false;
        } else if (member == "color") {
            if (!parse_color(jimp, &t.color)) return false;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member: %s", (ZString)jimp.string);
            return false;
        }
    }

    return true;
}

fn bool parse_rect(Jimp *jimp, Rect *r) @local
{
    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "position") {
            if (!parse_vec2(jimp, &r.position)) return false;
        } else if (member == "size") {
            if (!parse_vec2(jimp, &r.size)) return false;
        } else if (member == "color") {
            if (!parse_color(jimp, &r.color)) return false;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member: %s", (ZString)jimp.string);
            return false;
        }
    }

    return true;
}

fn bool parse_setup_item(Jimp *jimp, Obj *obj) @local
{
    if (!jimp::object_begin(jimp)) return false;

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "kind") {
            String kind;
            if (!parse_str(jimp, &kind)) return false;
            switch (kind) {
                case "text":
                    obj.kind = ObjKind.TEXT;
                case "rect":
                    obj.kind = ObjKind.RECT;
                default:
                    io::printfn("ERROR: Unknown kind in setup: %s", kind);
                    return false;
            }
        } else if (member == "obj_id") {
            if (!jimp::number(jimp)) return false;
            obj.obj_id = (int)jimp.number;
        } else if (member == "props") {
            if (!jimp::object_begin(jimp)) return false;

            switch (obj.kind) {
                case ObjKind.TEXT:
                    if (!parse_text(jimp, &obj.as.text)) return false;
                case ObjKind.RECT:
                    if (!parse_rect(jimp, &obj.as.rect)) return false;
            }

            if (!jimp::object_end(jimp)) return false;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member in setup item: %s", (ZString)jimp.string);
            return false;
        }
    }

    return jimp::object_end(jimp);
}

fn bool parse_v2_interp(Jimp *jimp, Vector2Interp *vi) @local
{
    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "start") {
            if (!parse_vec2(jimp, &vi.start)) return false;
        } else if (member == "end") {
            if (!parse_vec2(jimp, &vi.end)) return false;
        } else if (member == "obj_id") {
            if (!jimp::number(jimp)) return false;
            vi.obj_id = (int)jimp.number;
        } else if (member == "prop_name") {
            if (!parse_str(jimp, &vi.prop_name)) return false;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member in vector2 interp item: %s", (ZString)jimp.string);
            return false;
        }
    }

    return true;
}

fn bool parse_clr_interp(Jimp *jimp, ColorInterp *ci) @local
{
    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "start") {
            if (!parse_color(jimp, &ci.start)) return false;
        } else if (member == "end") {
            if (!parse_color(jimp, &ci.end)) return false;
        } else if (member == "obj_id") {
            if (!jimp::number(jimp)) return false;
            ci.obj_id = (int)jimp.number;
        } else if (member == "prop_name") {
            if (!parse_str(jimp, &ci.prop_name)) return false;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member in color interp item: %s", (ZString)jimp.string);
            return false;
        }
    }

    return true;
}

fn bool parse_action_item(Jimp *jimp, Action *action) @local
{
    if (!jimp::object_begin(jimp)) return false;

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "type") {
            String kind;
            if (!parse_str(jimp, &kind)) return false;
            switch (kind) {
                case "v2Interp":
                    action.kind = ActionKind.VECTOR2_INTERP;
                case "clrInterp":
                    action.kind = ActionKind.COLOR_INTERP;
                default:
                    io::printfn("ERROR: Unknown kind in actions: %s", kind);
                    return false;
            }
        } else if (member == "action_id") {
            if (!jimp::number(jimp)) return false;
            action.action_id = (int)jimp.number;
        } else if (member == "duration") {
            if (!jimp::number(jimp)) return false;
            action.duration = (float)jimp.number;
        } else if (member == "props") {
            if (!jimp::object_begin(jimp)) return false;

            // TODO: hard coding this type here is wrong
            switch (action.kind) {
                case ActionKind.VECTOR2_INTERP:
                    if (!parse_v2_interp(jimp, &action.as.v2_interp)) return false;
                case ActionKind.COLOR_INTERP:
                    if (!parse_clr_interp(jimp, &action.as.clr_interp)) return false;
            }

            if (!jimp::object_end(jimp)) return false;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member in action item: %s", (ZString)jimp.string);
            return false;
        }
    }

    return jimp::object_end(jimp);
}

fn bool? resolve_action_value_ptrs(Action[] actions, Obj[] objs) @local
{
    foreach (&action : actions) {
        switch (action.kind) {
            case ActionKind.VECTOR2_INTERP:
                Vector2Interp* vi = &action.as.v2_interp;
                Obj *obj = &objs[vi.obj_id];
                any? value_ptr = obj.get_prop_by_name(vi.prop_name)!;
                vi.value = anycast(value_ptr, rl::Vector2)!;
            case ActionKind.COLOR_INTERP:
                ColorInterp* ci = &action.as.clr_interp;
                Obj *obj = &objs[ci.obj_id];
                any? value_ptr = obj.get_prop_by_name(ci.prop_name)!;
                ci.value = anycast(value_ptr, rl::Color)!;
        }
    }

    io::printn("TODO: resolve_action_value_ptrs() unimplemented");
    return true;
}

fn bool parse_config(Jimp *jimp, Config *cfg) @local
{
    if (!jimp::object_begin(jimp)) return false;

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "resolution") {
            if (!parse_vec2(jimp, &cfg.resolution)) return false;
        } else if (member == "outputPath") {
            if (!parse_str(jimp, &cfg.output_path)) return false;
        } else if (member == "fps") {
            if (!jimp::number(jimp)) return false;
            cfg.fps = (int)jimp.number;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member: %s", (ZString)jimp.string);
            return false;
        }
    }

    return jimp::object_end(jimp);
}

fn bool parse_script(Jimp *jimp, Config *cfg, List{Obj} *objs, List{Action} *actions) @local
{
    if (!jimp::object_begin(jimp)) return false;

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        io::printn(member);
        if (member == "config") {
            if (!parse_config(jimp, cfg)) return false;
        } else if (member == "setup") {
            if (!jimp::array_begin(jimp)) return false;
            Obj obj;
            while (jimp::array_item(jimp)) {
                if (!parse_setup_item(jimp, &obj)) return false;
                objs.push(obj);
            }
            if (!jimp::array_end(jimp)) return false;
        } else if (member == "actions") {
            if (!jimp::array_begin(jimp)) return false;
            Action action;
            while (jimp::array_item(jimp)) {
                if (!parse_action_item(jimp, &action)) return false;
                actions.push(action);
            }
            if (!jimp::array_end(jimp)) return false;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member: %s", member);
            return false;
        }
    }

    if (!jimp::object_end(jimp)) return false;
    return true;
}


fn void main(String[] args)
{
    if (args.len == 1) {
        String program_name = args[0];
        io::printfn("USAGE: %s <INPUT_SCRIPT>", program_name);
        return;
    }

    String script_path = args[1];
    char[1024] buffer;
    File? f = file::open(script_path, "r")!!;
    defer (void) f.close();
    f.read(buffer[..])!!;
    ZString content = (ZString)buffer[..];

    Jimp jimp;
    jimp::begin(&jimp, script_path, content, content.len());

    Config cfg;
    List{Obj} objs;
    List{Action} actions;

    bool result = parse_script(&jimp, &cfg, &objs, &actions);
    if (!result) {
        io::printn("Failed to parse script");
        return;
    }

    io::print("Config: ");
    io::printn(cfg);

    foreach (obj : objs) {
        switch (obj.kind) {
            case ObjKind.TEXT:
                io::printn(obj.as.text);
            case ObjKind.RECT:
                io::printn(obj.as.rect);
        }
    }

    foreach (action : actions) {
        switch (action.kind) {
            case ActionKind.VECTOR2_INTERP:
                io::printn(action.as.v2_interp);
            case ActionKind.COLOR_INTERP:
                io::printn(action.as.clr_interp);
        }
    }

    if (catch excuse = resolve_action_value_ptrs(actions.array_view(), objs.array_view())) {
        io::printn(excuse);
        return;
    }

    rl::set_config_flags(ConfigFlags.FLAG_MSAA_4X_HINT.code);
    rl::init_window((int)cfg.resolution.x, (int)cfg.resolution.y, "Hello, from c3");
    rl::set_target_fps(cfg.fps);
    defer rl::close_window();

    while (!rl::window_should_close()) {
        foreach (&action : actions) {
            action.step(rl::get_frame_time());
        }

        rl::begin_drawing();
            rl::clear_background(rl::get_color(0x181818FF));
            foreach (&obj : objs) {
                obj.rl_render();
            }
        rl::end_drawing();
    }
}

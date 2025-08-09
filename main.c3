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

fn bool parse_setup_item(Jimp *jimp, Obj *obj)
{
    if (!jimp::object_begin(jimp)) return false;

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "kind") {
            if (!jimp::string(jimp)) return false;
            String kind = ((ZString)jimp.string).str_view();
            switch (kind) {
                case "text":
                    obj.kind = ObjKind.TEXT;
                default:
                    io::printfn("ERROR: Unknown kind in setup: %s", kind);
                    return false;
            }
        } else if (member == "id") {
            if (!jimp::number(jimp)) return false;
            obj.id = (int)jimp.number;
        } else if (member == "props") {
            if (!jimp::object_begin(jimp)) return false;

            Text *t = &obj.as.text;
            while (jimp::object_member(jimp)) {
                member = (ZString)jimp.string;
                if (member == "text") {
                    if (!jimp::string(jimp)) return false;
                    t.text = ((ZString)jimp.string).tcopy();
                } else if (member == "fontSize") {
                    if (!jimp::number(jimp)) return false;
                    t.font_size = (float)jimp.number;
                } else if (member == "position") {
                    if (!jimp::array_begin(jimp)) return false;
                    int i = 0;
                    while (i < rl::Vector2.len && jimp::array_item(jimp)) {
                        if (!jimp::number(jimp)) return false;
                        t.position[i++] = (float)jimp.number;
                    }
                    if (!jimp::array_end(jimp)) return false;
                } else if (member == "color") {
                    if (!jimp::array_begin(jimp)) return false;
                    int i = 0;
                    while (i < rl::Color.len && jimp::array_item(jimp)) {
                        if (!jimp::number(jimp)) return false;
                        t.color[i++] = (char)jimp.number;
                    }
                    if (!jimp::array_end(jimp)) return false;
                } else {
                    jimp::unknown_member(jimp);
                    io::printfn("ERROR: Unknown member: %s", (ZString)jimp.string);
                    return false;
                }
            }

            if (!jimp::object_end(jimp)) return false;
        } else {
            jimp::unknown_member(jimp);
            io::printfn("ERROR: Unknown member: %s", (ZString)jimp.string);
            return false;
        }
    }

    return jimp::object_end(jimp);
}

fn bool parse_json(String json_path, Jimp *jimp, List{Obj} *objs)
{
    if (!jimp::object_begin(jimp)) return false;

    ZString member;
    int i = 0;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "setup") {
            if (!jimp::array_begin(jimp)) return false;
            Obj obj;
            while (jimp::array_item(jimp)) {
                if (!parse_setup_item(jimp, &obj)) return false;
                objs.push(obj);
            }
            if (!jimp::array_end(jimp)) return false;
        }
    }

    if (!jimp::object_end(jimp)) return false;
    return true;
}

fn void main()
{
    String json_path = "./first.json";
    char[1024] buffer;
    File? f = file::open(json_path, "r")!!;
    f.read(buffer[..])!!;
    ZString content = (ZString)buffer[..];

    Jimp jimp;
    jimp::begin(&jimp, json_path, content, content.len());
    io::printn("----");
    io::printn(content);
    io::printn("----");

    List{Obj} objs;

    bool result = parse_json(json_path, &jimp, &objs);
    if (result) {
        foreach (obj : objs) {
            switch (obj.kind) {
                case ObjKind.TEXT:
                    io::printn(obj.as.text);
                default:
                    io::printfn("ERROR: Unknown obj.kind (%s)", obj.kind);
                    return;
            }
        }
    }

    // char[1024] buffer;
    // File? f = file::open(json_path, "r")!!;
    // f.read(buffer[..])!!;
    // ZString json_content = (ZString)buffer[..];
    // Object*? os = json::tparse_string(json_content.str_view());

    // Object*? objs = json::tparse_string(json_content.str_view());
    // if (catch excuse = objs) {
    //     io::printn(json_content);
    //     io::printfn("ERROR: %s", excuse);
    //     return;
    // }
    // io::printn(objs);

    // rl::set_config_flags(ConfigFlags.FLAG_MSAA_4X_HINT.code);
    // rl::init_window(800, 600, "Hello, from c3");
    // rl::set_target_fps(60);
    // defer rl::close_window();
    //
    // while (!rl::window_should_close()) {
    //     rl::begin_drawing();
    //         rl::clear_background(rl::get_color(0x181818FF));
    //     rl::end_drawing();
    // }
}

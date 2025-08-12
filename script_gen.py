import json, os, sys

class Vector2:
    def __init__(self, x: float, y: float) -> None:
        self.x: float = x
        self.y: float = y

    def copy(self) -> 'Vector2':
        return Vector2(self.x, self.y)

    def as_list(self) -> list[float]:
        return [self.x, self.y]

class Color:
    def __init__(self, r: int, g: int, b: int, a: int) -> None:
        self.r: int = max(min(r, 255), 0)
        self.g: int = max(min(g, 255), 0)
        self.b: int = max(min(b, 255), 0)
        self.a: int = max(min(a, 255), 0)

    def as_list(self) -> list[int]:
        return [self.r, self.g, self.b, self.a]

class Obj:
    obj_kinds = ["text", "rect"]
    def __init__(self, kind: str) -> None:
        assert kind in Obj.obj_kinds, f"(kind: {kind}) not in (Obj.objs_kind: {Obj.obj_kinds})"

        self.kind: str = kind
        self.id: int | None = None

    def set_id(self, id: int) -> None:
        self.id = id

    def as_dict(self) -> dict:
        assert self.id is not None, f"obj id must not be None. Maybe add it to a Video object"
        return {
            "kind": self.kind,
            "obj_id": self.id
        }

class Text(Obj):
    def __init__(self, text: str, font_size: float, position: Vector2, color: Color) -> None:
        super().__init__("text")
        self.text: str = text
        self.font_size: float = font_size
        self.position: Vector2 = position
        self.color: Color = color

    def as_dict(self) -> dict:
        return {
            **super().as_dict(),
            "props": {
                "text": self.text,
                "fontSize": self.font_size,
                "position": self.position.as_list(),
                "color": self.color.as_list()
            }
        }

class Rect(Obj):
    def __init__(self, position: Vector2, size: Vector2, color: Color) -> None:
        super().__init__("rect")
        self.position: Vector2 = position
        self.size: Vector2 = size
        self.color: Color = color

    def as_dict(self) -> dict:
        return {
            **super().as_dict(),
            "props": {
                "position": self.position.as_list(),
                "size": self.size.as_list(),
                "color": self.color.as_list()
            }
        }


class Action:
    action_kinds = ["clrInterp", "v2Interp"]
    def __init__(self, kind: str, duration: float) -> None:
        assert kind in Action.action_kinds, (
            f"(kind: {kind}) not in (Action.action_kinds: {Action.action_kinds})"
        )

        self.kind: str = kind
        self.duration = duration
        self.id: int | None = None

    def set_id(self, id: int) -> None:
        self.id = id

    def as_dict(self) -> dict:
        assert self.id is not None
        return {
            "kind": self.kind,
            "action_id": self.id,
            "duration": self.duration
        }


class ColorInterp(Action):
    def __init__(self, start: Color, end: Color, obj_id: int | None, prop_name: str, duration: float) -> None:
        assert obj_id is not None, f"obj id must not be None. Maybe add it to a Video object"

        super().__init__("clrInterp", duration)
        self.start: Color = start
        self.end: Color = end
        self.obj_id: int = obj_id
        self.prop_name: str = prop_name

    def as_dict(self) -> dict:
        return {
            **super().as_dict(),
            "props": {
                "start": self.start.as_list(),
                "end": self.end.as_list(),
                "obj_id": self.obj_id,
                "prop_name": self.prop_name
            }
        }


class Vector2Interp(Action):
    def __init__(self, start: Vector2, end: Vector2, obj_id: int | None, prop_name: str, duration: float) -> None:
        assert obj_id is not None, f"obj id must not be None. Maybe add it to a Video object"

        super().__init__("v2Interp", duration)
        self.start: Vector2 = start
        self.end: Vector2 = end
        self.obj_id: int = obj_id
        self.prop_name: str = prop_name

    def as_dict(self) -> dict:
        return {
            **super().as_dict(),
            "props": {
                "start": self.start.as_list(),
                "end": self.end.as_list(),
                "obj_id": self.obj_id,
                "prop_name": self.prop_name
            }
        }


class Video:
    def __init__(self, width: int, height: int, output_path: str, fps: int = 30) -> None:
        self.width: int = width
        self.height: int = height
        self.output_path: str = output_path
        self.fps: int = fps

        self.objs: list[Obj] = []
        self.obj_id_counter: int = 0

        self.actions: list[Action] = []
        self.action_id_counter: int = 0

    def add_obj(self, obj: Obj) -> None:
        obj.set_id(self.obj_id_counter)
        self.objs.append(obj)
        self.obj_id_counter += 1

    def add_action(self, action: Action) -> None:
        action.set_id(self.obj_id_counter)
        self.actions.append(action)
        self.action_id_counter += 1

    def as_dict(self) -> dict:
        return {
            # "config": {
            #     "width": self._width,
            #     "height": self._height,
            #     "outputPath": self._output_path,
            #     "fps": 30
            # },
            "objs": [ obj.as_dict() for obj in self.objs ],
            "actions": [ action.as_dict() for action in self.actions ]
        }

    def to_json(self, output_path: str, overwrite: bool = False) -> None:
        if os.path.exists(output_path):
            if overwrite:
                print(f"WARNING: Overwriting {output_path}...")
            else:
                print(f"WARNING: {output_path} already exists")
                sys.exit(1)

        with open(output_path, "w") as f:
            json.dump(self.as_dict(), f, indent=4)


def main():
    v = Video(800, 600, "out1.mov", fps=30)

    t = Text("Hello, world!", 32, Vector2(123, 321), Color(249, 134, 102, 255))
    v.add_obj(t)

    ci = ColorInterp(t.color, Color(0, 255, 0, 255), t.id, "color", 3)
    v.add_action(ci)

    vi = Vector2Interp(t.position, Vector2(321, 123), t.id, "position", 1)
    v.add_action(vi)

    v.to_json("test.json", overwrite=True)

if __name__ == "__main__":
    main()

import unreal


MAP_PATH = "/Game/Maps/SeedForgeDemo"


def main() -> None:
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        if not level_editor.load_level(MAP_PATH):
            raise RuntimeError(f"Unable to load existing demo map: {MAP_PATH}")
    else:
        if not level_editor.new_level(MAP_PATH):
            raise RuntimeError(f"Unable to create demo map: {MAP_PATH}")

    if not level_editor.save_current_level():
        raise RuntimeError(f"Unable to save demo map: {MAP_PATH}")

    unreal.log(f"SEEDFORGE_MAP_READY {MAP_PATH}")


main()


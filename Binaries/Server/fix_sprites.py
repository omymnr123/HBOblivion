import json

json_path = r"d:\HB Server\Helbreath-Heldenian-Project-Development\Binaries\Game\CONTENTS\ItemSpriteMetadata.json"

with open(json_path, "r") as f:
    data = json.load(f)

# Remove the bad entries (id 5000 and 5001)
data = [d for d in data if d.get("id") not in (5000, 5001)]

# Add correct entries for non-equippable items
# User said: ground=363/364, inventory=385/386
data.append({
    "id": 5000,
    "equip_type": "none",
    "pak_file": None,
    "inventory_frame_index": 385,
    "ground_frame_index": 363
})

data.append({
    "id": 5001,
    "equip_type": "none",
    "pak_file": None,
    "inventory_frame_index": 386,
    "ground_frame_index": 364
})

with open(json_path, "w") as f:
    json.dump(data, f, indent=4)

print("Fixed ItemSpriteMetadata.json")
print(f"Total entries: {len(data)}")

# Verify the entries
for d in data:
    if d.get("id") in (5000, 5001):
        print(f"  id={d['id']}: inventory_frame={d.get('inventory_frame_index')}, ground_frame={d.get('ground_frame_index')}")

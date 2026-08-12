import sqlite3

paths = [
    r'd:\HB Server\Helbreath-Heldenian-Project-Development\Binaries\Server\gamedata.db',
    r'd:\HB Server\Helbreath-Heldenian-Project-Development\Sources\Release\gamedata.db',
]

names = [
    "Aresden-Hero's Cape",
    "Aresden Hero Helm (M)",
    "Aresden Hero Helm (W)",
    "Aresden Hero Cap (M)",
    "Aresden Hero Cap (W)",
    "Aresden Hero Armor (M)",
    "Aresden Hero Armor (W)",
    "Aresden Hero Robe (M)",
    "Aresden Hero Robe (W)",
    "Aresden Hero Hauberk (M)",
    "Aresden Hero Hauberk (W)",
    "Aresden Hero Leggings (M)",
    "Aresden Hero Leggings (W)",
    "Aresden-Hero's Cape +1",
    "Elvine-Hero's Cape",
    "Elvine Hero Helm (M)",
    "Elvine Hero Helm (W)",
    "Elvine Hero Cap (M)",
    "Elvine Hero Cap (W)",
    "Elvine Hero Armor (M)",
    "Elvine Hero Armor (W)",
    "Elvine Hero Robe (M)",
    "Elvine Hero Robe (W)",
    "Elvine Hero Hauberk (M)",
    "Elvine Hero Hauberk (W)",
    "Elvine Hero Leggings (M)",
    "Elvine Hero Leggings (W)",
    "Elvine-Hero's Cape +1",
]

for path in paths:
    try:
        con = sqlite3.connect(path)
        cur = con.cursor()
        cur.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='items'")
        if not cur.fetchone():
            print(path, 'SKIP: no items table')
            con.close()
            continue

        placeholders = ','.join('?' for _ in names)
        cur.execute(f"UPDATE items SET is_dyeable = 1, armor_class = 2 WHERE name IN ({placeholders})", names)
        con.commit()
        cur.execute(f"SELECT item_id, name, is_dyeable, armor_class FROM items WHERE name IN ({placeholders}) ORDER BY item_id", names)
        print(path)
        for row in cur.fetchall():
            print(row)
        con.close()
    except Exception as e:
        print(path, 'ERROR', e)

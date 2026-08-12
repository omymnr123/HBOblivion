import sqlite3
con = sqlite3.connect('Binaries/Server/gamedata.db')
cur = con.cursor()
print('tables', cur.execute("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name").fetchall())
cur.execute("SELECT item_id, name, is_dyeable, armor_class, item_effect_type FROM items WHERE item_id IN (752,753,756,757) ORDER BY item_id")
for row in cur.fetchall():
    print(row)
con.close()

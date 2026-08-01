import sqlite3
path = 'Binaries/Server/gamedata.db'
con = sqlite3.connect(path)
cur = con.cursor()
ids = [752, 753, 756, 757]
for item_id in ids:
    cur.execute("SELECT is_dyeable, armor_class FROM items WHERE item_id = ?", (item_id,))
    print('before', item_id, cur.fetchone())
    cur.execute("UPDATE items SET armor_class = 2 WHERE item_id = ?", (item_id,))
    con.commit()
    cur.execute("SELECT is_dyeable, armor_class FROM items WHERE item_id = ?", (item_id,))
    print('after ', item_id, cur.fetchone())
con.close()

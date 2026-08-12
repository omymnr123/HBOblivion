import sqlite3
conn = sqlite3.connect(r'd:\HB Server\Helbreath-Heldenian-Project-Development\Binaries\Server\gamedata.db')
cursor = conn.cursor()
cursor.execute('SELECT item_id, name, display_id FROM items WHERE name LIKE "%Experience Potion%" OR name LIKE "%Dye%"')
for row in cursor.fetchall(): print(row)

from paklib import PAKFile
from pathlib import Path

def main():
    # Lista con los nombres de tus carpetas
    carpetas = ["Sarcofago_1", "Sarcofago_2", "Sarcofago_3"]
    
    print("======================================")
    print("EMPAQUETANDO NPCs...")
    
    for nombre in carpetas:
        carpeta = Path(nombre)
        if not carpeta.exists():
            print(f"ERROR: No se encuentra la carpeta {nombre}")
            continue
            
        salida = Path(f"{nombre}.pak")
        
        # Lee el PNG y el JSON y crea el PAK
        pak = PAKFile.from_directory(carpeta, "sprite")
        pak.write(salida)
        print(f"¡Éxito! -> Creado {salida.name}")

    print("======================================")

if __name__ == "__main__":
    main()
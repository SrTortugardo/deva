# imports
import os
import struct
import sys
from pathlib import Path

# constantes
DEVAFS_MAGIC = 0x61766564 # aved en little endian
MAX_FILES = 64
MAX_FILENAME = 32
MAX_FILE_SIZE = 8192 # 8kb
SECTOR_SIZE = 512
FS_START_SECTOR = 2048

def help():
    MSG = """
diskutil.py : sirve para crear una imagen del sistema de archivos devafs para que sea leido por mi kernel

Uso de Diskutil :
    python3 diskutil <directorio de entrada> <directorio de salida>
    """

    print(MSG)

def add_directory(fs_header, fs_data, dir_path, parent_slot, depth=0):
    if depth > 10:
        return

    dir_path = Path(dir_path)
    if not dir_path.is_dir():
        return

    print(f"Escaneando: {dir_path}")

    try:
        entries = sorted(dir_path.iterdir(), key=lambda e: e.name)
    except OSError:
        return

    for entry in entries:
        name = str(entry.name)
        full_path = str(entry)

        try:
            metadata = entry.stat()
        except OSError:
            continue

        slot = find_free_slot(fs_header)
        if slot is None:
            continue

        fs_entry = fs_header["files"][slot]
        fs_entry["used"] = 1
        fs_entry["parent"] = parent_slot

        if entry.is_dir():
            fs_entry["is_dir"] = 1
            name_bytes = name.encode("utf-8")
            name_len = min(len(name_bytes), MAX_FILENAME - 1)
            fs_entry["name"][:name_len] = name_bytes[:name_len]
            print(f"Dir: {name}")
            add_directory(fs_header, fs_data, full_path, slot, depth + 1)
        else:
            fs_entry["is_dir"] = 0

            name_bytes = name.encode("utf-8")
            name_len = min(len(name_bytes), MAX_FILENAME - 1)
            fs_entry["name"][:name_len] = name_bytes[:name_len]

            try:
                with open(full_path, "rb") as f:
                    content = f.read()
            except OSError:
                fs_entry["used"] = 0
                continue

            content_len = len(content)
            if content_len > MAX_FILE_SIZE:
                print(f"Omitiendo {name} (archivo grande)")
                fs_entry["used"] = 0
                continue

            fs_entry["size"] = content_len
            fs_entry["offset"] = slot * MAX_FILE_SIZE

            file_start = slot * MAX_FILE_SIZE
            file_end = file_start + content_len
            fs_data[file_start:file_end] = content

            print(f"Archivo: {name} de ({content_len} bytes)")

def find_free_slot(fs_header):
    for i in range(MAX_FILES):
        if not fs_header["files"][i]["used"]:
            return i
    return None

def count_files(fs_header):
    count = 0
    for entry in fs_header["files"]:
        if entry["used"] and not entry["is_dir"]:
            count += 1
    return count

def write_image(fs_header, fs_data, output):
    SECTOR_SIZE = 512
    FS_START_SECTOR = 2048
    MAX_FILES = 64
    MAX_FILENAME = 32

    # Layout packed que espera el kernel (ver kernel/include/devafs.h):
    #   file_entry_t  = name[32] + size + offset + used + is_dir + parent = 46 bytes
    #   devafs_header_t = magic + file_count + files[MAX_FILES] = 8 + 64*46 = 2952 bytes
    FILE_ENTRY_FMT = "<32sIIBBI"                       # 46 bytes, little-endian sin padding
    FILE_ENTRY_SIZE = struct.calcsize(FILE_ENTRY_FMT)  # 46
    DEVAFS_HEADER_SIZE = 8 + FILE_ENTRY_SIZE * MAX_FILES        # 2952
    HEADER_SECTORS = (DEVAFS_HEADER_SIZE + SECTOR_SIZE - 1) // SECTOR_SIZE  # 6

    total_offset_to_fs_start = FS_START_SECTOR * SECTOR_SIZE
    data_start_offset = total_offset_to_fs_start + HEADER_SECTORS * SECTOR_SIZE

    data_len = len(fs_data)
    total_size = data_start_offset + data_len

    with open(output, "wb") as f:
        # 1. El kernel espera el header en FS_START_SECTOR (sector 2048)
        f.seek(total_offset_to_fs_start)

        # 2. Construir el header COMPLETO: magic + file_count + tabla de entradas
        header_bytes = struct.pack("<II", fs_header["magic"], fs_header["file_count"])
        for entry in fs_header["files"]:
            name_bytes = bytes(entry["name"])
            if len(name_bytes) < MAX_FILENAME:
                name_bytes = name_bytes + b"\x00" * (MAX_FILENAME - len(name_bytes))
            elif len(name_bytes) > MAX_FILENAME:
                name_bytes = name_bytes[:MAX_FILENAME]
            header_bytes += struct.pack(
                FILE_ENTRY_FMT,
                name_bytes,
                entry["size"],
                entry["offset"],
                entry["used"],
                entry["is_dir"],
                entry["parent"],
            )

        f.write(header_bytes)

        # 3. Pad hasta el limite de HEADER_SECTORS (por si el header no es multiplo)
        current_pos = f.tell()
        if current_pos < data_start_offset:
            f.write(b"\x00" * (data_start_offset - current_pos))

        # 4. Escribir los datos de los archivos (fs_data tiene cada archivo en slot*MAX_FILE_SIZE)
        f.write(fs_data)

    sectors = (total_size + SECTOR_SIZE - 1) // SECTOR_SIZE
    print(f"Escrito a: {output}")
    print(f"Tamaño de la imagen: {sectors} sectores ({total_size} bytes)")

## main
arg_len = len(sys.argv)
if arg_len < 3:
    help() # i need somebody
    sys.exit(1)

source = sys.argv[1]
output = sys.argv[2]

if not os.path.isdir(source):
    print("No existe ese tal directorio que pasaste de entrada")
    sys.exit(1)

source_path = Path(source)

fs_header = {
    "magic": DEVAFS_MAGIC,
    "file_count": 0,
    "files": [
        {
            "name": bytearray(MAX_FILENAME),
            "size": 0,
            "offset": 0,
            "used": 0,
            "is_dir": 0,
            "parent": 0,
        }
        for _ in range(MAX_FILES)
    ],
}

fs_data = bytearray(MAX_FILES * MAX_FILE_SIZE)

add_directory(fs_header, fs_data, source_path, 0, 0)

file_count = count_files(fs_header)
fs_header["file_count"] = file_count
print(f"Total de archivos: {file_count}")
write_image(fs_header, fs_data, output)

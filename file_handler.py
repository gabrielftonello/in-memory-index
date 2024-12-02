import struct
from dataclasses import dataclass

import b_plus_tree
import hash_table

ORIGINAL_FILE_NAME = "./products.bin"

HEADER_FORMAT = '=qi4x'
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

PRODUCT_RECORD_FORMAT = '=qq64s32sfiqq'
PRODUCT_RECORD_SIZE = struct.calcsize(PRODUCT_RECORD_FORMAT)

assert HEADER_SIZE == 16, f"Tamanho Header incorreto: {HEADER_SIZE} bytes"
assert PRODUCT_RECORD_SIZE == 136, f"Tamanho ProductRecord incorreto: {PRODUCT_RECORD_SIZE} bytes"

@dataclass
class ProductRecord:
    product_id: int
    category_id: int
    category_code: str
    brand: str
    price: float
    ativo: int
    seq_key: int
    elo: int
    
    
def build_index_from_file(field_extractor, data_structure, batch_size=256):

    if data_structure is None or field_extractor is None:
        return

    with open(ORIGINAL_FILE_NAME, 'rb') as file:
       
        header_data = file.read(HEADER_SIZE)
        if len(header_data) != HEADER_SIZE:
            return

        while True:
            base_position = file.tell()
            batch_data = file.read(PRODUCT_RECORD_SIZE * batch_size)
            if not batch_data:
                break  
            num_records = len(batch_data) // PRODUCT_RECORD_SIZE
            for i in range(num_records):
                record_offset = i * PRODUCT_RECORD_SIZE
                record_data = batch_data[record_offset: record_offset + PRODUCT_RECORD_SIZE]
                if len(record_data) != PRODUCT_RECORD_SIZE:
                    break  

                field_value = field_extractor(record_data)
                if field_value is None:
                    continue  

                position = base_position + record_offset

                data_structure.insert(field_value, position)

def extract_price(record_data):
    try:
        price_offset = 112
        price_size = 4
        price_data = record_data[price_offset:price_offset + price_size]
        
        price = struct.unpack('=f', price_data)[0]
        return price
    except Exception as e:
        return None
    
def extract_product_id(record_data):
    try:
        product_id_data = record_data[0:8]
        product_id = struct.unpack('=q', product_id_data)[0]
        return product_id
    except Exception as e:
        return None

def extract_brand(record_data):
    try:
        brand_offset = 80  
        brand_size = 32    
        brand_data = record_data[brand_offset: brand_offset + brand_size]
        brand_bytes = brand_data.rstrip(b' \x00')
        brand = brand_bytes.decode('ascii', errors='ignore')
        return brand.lower()
    except Exception as e:
        return None
    
def extract_category_code(record_data):
    try:
        brand_offset = 16  
        brand_size = 64    
        brand_data = record_data[brand_offset: brand_offset + brand_size]
        brand_bytes = brand_data.rstrip(b' \x00')
        brand = brand_bytes.decode('ascii', errors='ignore')
        return brand.lower()
    except Exception as e:
        return None

def read_product_record_data_file(binary_pos):
    with open(ORIGINAL_FILE_NAME, 'rb') as file:
        file.seek(binary_pos)

        record_data = file.read(PRODUCT_RECORD_SIZE)
        if len(record_data) != PRODUCT_RECORD_SIZE:
            return

        unpacked_data = struct.unpack(PRODUCT_RECORD_FORMAT, record_data)

        product_id = unpacked_data[0]
        category_id = unpacked_data[1]

        category_code_bytes = unpacked_data[2].rstrip(b' \x00')
        category_code = category_code_bytes.decode('ascii', errors='ignore')

        brand_bytes = unpacked_data[3].rstrip(b' \x00')
        brand = brand_bytes.decode('ascii', errors='ignore')

        price = unpacked_data[4]
        ativo = unpacked_data[5]
        seq_key = unpacked_data[6]
        elo = unpacked_data[7]

        product_record = ProductRecord(
            product_id=product_id,
            category_id=category_id,
            category_code=category_code,
            brand=brand,
            price=price,
            ativo=ativo,
            seq_key=seq_key,
            elo=elo
        )

        return product_record

def update_product_record_data_file(binary_pos, updated_record: ProductRecord):
    with open(ORIGINAL_FILE_NAME, 'r+b') as file:
        file.seek(binary_pos)
        packed_data = struct.pack(
            PRODUCT_RECORD_FORMAT,
            updated_record.product_id,
            updated_record.category_id,
            updated_record.category_code.encode('ascii').ljust(64, b'\x00'),
            updated_record.brand.encode('ascii').ljust(32, b'\x00'),
            updated_record.price,
            updated_record.ativo,
            updated_record.seq_key,
            updated_record.elo
        )
        file.write(packed_data)

def get_last_seq_key():
    try:
        with open(ORIGINAL_FILE_NAME, 'rb') as file:
            file.seek(0, 2)  
            file_size = file.tell()
            
            if file_size < HEADER_SIZE + PRODUCT_RECORD_SIZE:
                return 0  
            
            file.seek(-PRODUCT_RECORD_SIZE, 2)
            record_data = file.read(PRODUCT_RECORD_SIZE)
            
            if len(record_data) != PRODUCT_RECORD_SIZE:
                return 0  
            
            unpacked_data = struct.unpack(PRODUCT_RECORD_FORMAT, record_data)
            last_seq_key = unpacked_data[6]  
            return last_seq_key
    except Exception as e:
        return 0

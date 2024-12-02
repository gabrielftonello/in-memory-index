import file_handler
import b_plus_tree
import hash_table
import sys
import struct
import time
from functools import wraps

def build_product_id_bplus_tree():
    bpt = b_plus_tree.BPlusTree(degree=257)
    start_time = time.perf_counter()
    file_handler.build_index_from_file(file_handler.extract_product_id, bpt)
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    print(f"Criacao da 'product_id_bplus_tree' em {duration_search:.6f} segundos.")
    
    return bpt

def build_price_bplus_tree():
    bpt_price = b_plus_tree.BPlusTree(degree=257)
    
    start_time = time.perf_counter()
    file_handler.build_index_from_file(file_handler.extract_price, bpt_price)
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    print(f"Criacao da 'price_bplus_tree' em {duration_search:.6f} segundos.")
    
    return bpt_price

def build_hash_tables(ht_brand, ht_category_code):
    start_time = time.perf_counter()
    file_handler.build_index_from_file(file_handler.extract_brand, ht_brand)
    end_time = time.perf_counter()
    duration_search = end_time - start_time
    print(f"Criacao da 'ht_brand' em {duration_search:.6f} segundos.")
    start_time = time.perf_counter()
    file_handler.build_index_from_file(file_handler.extract_category_code, ht_category_code)
    end_time = time.perf_counter()
    duration_search = end_time - start_time
    print(f"Criacao da 'ht_category_code' em {duration_search:.6f} segundos.")

def paginate_registers(positions):
    total = len(positions)
    batch_size = 10
    current = 0
    while current < total:
        end = min(current + batch_size, total)
        
        start_time = time.perf_counter()
        
        for pos in positions[current:end]:
            record = file_handler.read_product_record_data_file(pos)
            if record and record.ativo == 1:
                print(record)
        
        end_time = time.perf_counter()
        
        duration_search = end_time - start_time
        
        print(f"Registros lidos do arquivo de dados em {duration_search:.6f} segundos.")
        
        current += batch_size

        if current < total:
            while True:
                resposta = input("Deseja ver mais registros? (s/n): ").strip().lower()
                if resposta in ['s', 'n']:
                    break
                else:
                    print("Por favor, responda com 's' para sim ou 'n' para não.")
            
            if resposta == 'n':
                print("Encerrando a exibição de registros.")
                break
        else:
            print("Todos os registros foram exibidos.")
            
            
def search_by_brand(ht_brand, bpt):
    brand = input("Digite o nome da marca para buscar: ").strip().lower()
    if not brand:
        print("Marca inválida.")
        return

    start_time = time.perf_counter()
    positions = ht_brand.search(brand)
    end_time = time.perf_counter()
    duration_search = end_time - start_time
    print(f"Busca na tabela hash de 'brand' executada em {duration_search:.6f} segundos.")
    
    if not positions:
        print(f"Nenhum registro encontrado para a marca '{brand}'.")
        return

    print(f"Registros encontrados para a marca '{brand}':")
    
    paginate_registers(positions)

            
def search_by_category_code(ht_category_code, bpt):
    category_code = input("Digite o código da categoria para buscar: ").strip().lower()
    if not category_code:
        print("Código da categoria inválido.")
        return

    start_time = time.perf_counter()
    positions = ht_category_code.search(category_code)
    end_time = time.perf_counter()
    duration_search = end_time - start_time
    print(f"Busca na tabela hash de 'category_code' executada em {duration_search:.6f} segundos.")
    
    if positions is None:
        print(f"Nenhum registro encontrado para o código da categoria '{category_code}'.")
        return

    print(f"Registros encontrados para a categoria '{category_code}':")
    paginate_registers(positions)

def add_record(bpt_product_id, bpt_price, ht_brand, ht_category_code):
    
    try:
        product_id = int(input("Digite o Product ID: ").strip())
    except ValueError:
        print("Product ID inválido. Deve ser um número inteiro.")
        return

    existing_positions = bpt_product_id.retrieve(product_id)
    if existing_positions:
        pos = existing_positions[0]
        record = file_handler.read_product_record_data_file(pos)
        
        if record:
            if record.ativo == 1:
                print(f"Product ID {product_id} já existe e está ativo. Adição rejeitada.")
                return
            else:
                record.ativo = 1
                file_handler.update_product_record_data_file(pos, record)
                print(f"Registro com Product ID {product_id} reativado com sucesso.")
                
                bpt_product_id.insert(record.product_id, pos)
                bpt_price.insert(record.price, pos)
                ht_brand.insert(record.brand, pos)
                ht_category_code.insert(record.category_code, pos)
                return
        else:
            print("Erro ao ler o registro existente. Adição rejeitada.")
            return
    
    try:
        category_id = int(input("Digite o Category ID: ").strip())
    except ValueError:
        print("Category ID inválido. Deve ser um número inteiro.")
        return

    category_code = input("Digite o Category Code (até 64 caracteres): ").strip()
    if len(category_code) > 64:
        print("Category Code muito longo. Será truncado para 64 caracteres.")
        category_code = category_code[:64]

    brand = input("Digite a Brand (até 32 caracteres): ").strip()
    if len(brand) > 32:
        print("Brand muito longa. Será truncada para 32 caracteres.")
        brand = brand[:32]

    try:
        price = float(input("Digite o Price: ").strip())
    except ValueError:
        print("Price inválido. Deve ser um número float.")
        return

    last_seq_key = file_handler.get_last_seq_key()
    seq_key = last_seq_key + 1
    elo = 0

    new_record = file_handler.ProductRecord(
        product_id=product_id,
        category_id=category_id,
        category_code=category_code,
        brand=brand,
        price=price,
        ativo=1,      
        seq_key=seq_key,
        elo=elo
    )

    packed_data = struct.pack(
        file_handler.PRODUCT_RECORD_FORMAT,
        new_record.product_id,
        new_record.category_id,
        new_record.category_code.encode('ascii').ljust(64, b'\x00'),
        new_record.brand.encode('ascii').ljust(32, b'\x00'),
        new_record.price,
        new_record.ativo,
        new_record.seq_key,
        new_record.elo
    )

    try:
        with open(file_handler.ORIGINAL_FILE_NAME, 'ab') as file:  
            file.seek(0, 2)  
            position = file.tell()
            file.write(packed_data)
    except Exception as e:
        print(f"Erro ao escrever no arquivo: {e}")
        return


    start_time = time.perf_counter()
    bpt_product_id.insert(new_record.product_id, position)
    end_time = time.perf_counter()

    duration_search = end_time - start_time
    
    print(f"Insercao em 'bpt_product_id' executada em {duration_search:.6f} segundos.")
    
    start_time = time.perf_counter() 
    bpt_price.insert(new_record.price, position)
    end_time = time.perf_counter()
    duration_search = end_time - start_time
    
    print(f"Insercao em 'bpt_price' executada em {duration_search:.6f} segundos.")
    
    start_time = time.perf_counter() 
    ht_brand.insert(new_record.brand, position)
    end_time = time.perf_counter()
    duration_search = end_time - start_time
    
    print(f"Insercao em 'ht_brand' executada em {duration_search:.6f} segundos.")
    
    start_time = time.perf_counter() 
    ht_category_code.insert(new_record.category_code, position)
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    
    print(f"Insercao em 'ht_category_code' executada em {duration_search:.6f} segundos.")  

    print(f"Registro com Product ID {product_id} adicionado com sucesso.")

    
def delete_record(ht_brand, ht_category_code, bpt_product_id, bpt_price):
    try:
        product_id = int(input("Digite o Product ID do registro a ser excluído: ").strip())
    except ValueError:
        print("Product ID inválido.")
        return

    positions = bpt_product_id.retrieve(product_id)
    if positions is None:
        print(f"Nenhum registro encontrado com Product ID {product_id}.")
        return

    pos = positions[0]
    record = file_handler.read_product_record_data_file(pos)
    if record is None or record.ativo == 0:
        print(f"Registro com Product ID {product_id} já está inativo ou não existe.")
        return

    record.ativo = 0
    file_handler.update_product_record_data_file(pos, record)
    print(f"Registro com Product ID {product_id} foi marcado como inativo.")

    start_time = time.perf_counter() 
    ht_brand.remove(record.brand, pos)
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    print(f"Remocao em 'ht_brand' executada em {duration_search:.6f} segundos.")
    
    start_time = time.perf_counter() 
    ht_category_code.remove(record.category_code, pos)
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    print(f"Remocao em 'ht_category_code' executada em {duration_search:.6f} segundos.")
    
    print(f"Registro removido das tabelas hash de 'brand' e 'category_code'.")

    start_time = time.perf_counter() 
    bpt_product_id.delete(product_id)
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    print(f"Remocao em 'bpt_product_id' executada em {duration_search:.6f} segundos.")
    
    print(f"Registro removido da árvore B+ de Product ID.")

    start_time = time.perf_counter() 
    bpt_price.delete(record.price)
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    print(f"Remocao em 'bpt_price' executada em {duration_search:.6f} segundos.")
    
    print(f"Registro removido da árvore B+ de Price.")

def search_by_product_id(bpt_product_id):
    try:
        product_id = int(input("Digite o Product ID para buscar: ").strip())
    except ValueError:
        print("Product ID inválido.")
        return

    start_time = time.perf_counter()
     
    positions = bpt_product_id.retrieve(product_id)
    if positions is None:
        print(f"Nenhum registro encontrado com Product ID {product_id}.")
        return

    print(f"Registro encontrado para o Product ID {product_id}:")
    for pos in positions:
        record = file_handler.read_product_record_data_file(pos)
        if record and record.ativo == 1:
            print(record)
    
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    print(f"Selecao de 'product_id' em 'bpt_product_id' executada em {duration_search:.6f} segundos.")
            
def search_by_price_range(bpt_price):
    try:
        price_min = float(input("Digite o preço mínimo: ").strip())
        price_max = float(input("Digite o preço máximo: ").strip())
    except ValueError:
        print("Intervalo de preço inválido.")
        return

    if price_min > price_max:
        print("Preço mínimo não pode ser maior que o preço máximo.")
        return

    start_time = time.perf_counter()
    positions = bpt_price.retrieve_range(price_min, price_max)
    end_time = time.perf_counter()
    
    duration_search = end_time - start_time
    print(f"Selecao de 'price' em 'bpt_price' executada em {duration_search:.6f} segundos.")

    if not positions:
        print(f"Nenhum registro encontrado no intervalo de preço {price_min} a {price_max}.")
        return

    print(f"Registros encontrados no intervalo de preço {price_min} a {price_max}:")
    
    paginate_registers(positions)

    
def display_menu():
    print("\n1. Buscar registros por Brand")
    print("2. Buscar registros por Category Code")
    print("3. Buscar registro por Product ID")
    print("4. Buscar registros por intervalo de preço")
    print("5. Excluir um registro por Product ID")
    print("6. Adicionar um novo registro")
    print("7. Sair\n")

    
def main():
    print("Construindo índices, por favor aguarde...")
    bpt_product_id = build_product_id_bplus_tree() 
    bpt_price = build_price_bplus_tree() 
    ht_brand = hash_table.HashTable()
    ht_category_code = hash_table.HashTable()
    build_hash_tables(ht_brand, ht_category_code)
    print("Índices construídos com sucesso.")
    
    while True:
        display_menu()
        choice = input("Escolha uma opção: ").strip()

        if choice == '1':
            search_by_brand(ht_brand, bpt_price)
        elif choice == '2':
            search_by_category_code(ht_category_code, bpt_price)
        elif choice == '3':
            search_by_product_id(bpt_product_id)
        elif choice == '4':
            search_by_price_range(bpt_price)
        elif choice == '5':
            delete_record(ht_brand, ht_category_code, bpt_product_id, bpt_price)
        elif choice == '6':
            add_record(bpt_product_id, bpt_price, ht_brand, ht_category_code)
        elif choice == '7':
            print("Saindo do programa.")
            sys.exit(0)
        else:
            print("Opção inválida. Por favor, tente novamente.")
if __name__ == "__main__":
    main()

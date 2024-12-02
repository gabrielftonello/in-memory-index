#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#define MAX_CATEGORY_CODE_LEN 64
#define MAX_BRAND_LEN 32
#define CHUNK_SIZE 2
#define RECORDS_PER_PAGE 16
#define INDEX_INTERVAL 1024
#define FILE_ABSOLUTE_PATH "/Users/gabriel/Desktop/projects/python/"
#define ORIGINAL_FILE_NAME FILE_ABSOLUTE_PATH "products.bin"
#define SORTED_FILE_NAME FILE_ABSOLUTE_PATH "products_temp_sorted.bin"
#define INDEX_FILE_NAME FILE_ABSOLUTE_PATH "products.idx"

typedef struct
{
	long long head_index;
	int sequential_sorted;
}Header;

typedef struct
{
	long long product_id;
	long long category_id;
	char category_code[MAX_CATEGORY_CODE_LEN];
	char brand[MAX_BRAND_LEN];
	float price;
	int ativo;
	long long seq_key;
	long long elo;
}ProductRecord;

typedef struct
{
	long long product_id;
	long long seq_key;
}IndexRecord;

ProductRecord create_sample_product(long long product_id, long long category_id, const char *category_code_input, const char *brand_input, float price)
{
	ProductRecord record;
	record.product_id = product_id;
	record.category_id = category_id;

	size_t len = strlen(category_code_input);
	if (len >= MAX_CATEGORY_CODE_LEN - 1)
	{
		len = MAX_CATEGORY_CODE_LEN - 2;
	}

	strncpy(record.category_code, category_code_input, len);
	if (len < MAX_CATEGORY_CODE_LEN - 1)
	{
		memset(record.category_code + len, ' ', MAX_CATEGORY_CODE_LEN - len - 1);
	}

	record.category_code[MAX_CATEGORY_CODE_LEN - 1] = '\0';

	len = strlen(brand_input);
	if (len >= MAX_BRAND_LEN - 1)
	{
		len = MAX_BRAND_LEN - 2;
	}

	strncpy(record.brand, brand_input, len);
	if (len < MAX_BRAND_LEN - 1)
	{
		memset(record.brand + len, ' ', MAX_BRAND_LEN - len - 1);
	}

	record.brand[MAX_BRAND_LEN - 1] = '\0';

	record.price = price;
	record.ativo = 1;
	record.seq_key = 0;
	record.elo = 0;
	return record;
}

long long find_immediately_lower_product_id(long long target_product_id)
{
	FILE *fp = fopen(ORIGINAL_FILE_NAME, "rb");
	long long previous_index = -1;
	Header header;
	fread(&header, sizeof(Header), 1, fp);
	if (header.head_index != -1)
	{
		long long current_index = header.head_index;
		ProductRecord current_record;
		while (current_index != -1)
		{
			fseek(fp, sizeof(Header) + (current_index - 1) *sizeof(ProductRecord), SEEK_SET);
			fread(&current_record, sizeof(ProductRecord), 1, fp);
			if (current_record.product_id < target_product_id)
			{
				previous_index = current_index;
			}
			else
			{
				break;
			}

			current_index++;

			if (current_record.elo != 0)
				current_index = current_record.elo;
		}

		if (current_record.product_id == target_product_id)
		{
			previous_index = current_record.seq_key;
		}
	}

	fclose(fp);
	return previous_index;
}

int insert_record(const ProductRecord record)
{
	FILE *fp = fopen(ORIGINAL_FILE_NAME, "rb+");
	Header header;
	fread(&header, sizeof(Header), 1, fp);
	ProductRecord new_record = record;

	fseek(fp, 0, SEEK_END);
	long new_record_index = (ftell(fp) - sizeof(Header)) / sizeof(ProductRecord);
	new_record.seq_key = ++new_record_index;

	long lower_index = find_immediately_lower_product_id(new_record.product_id);

	if (lower_index == -1)
	{
		new_record.elo = header.head_index;
		fseek(fp, sizeof(Header) + (new_record_index - 1) *sizeof(ProductRecord), SEEK_SET);
		fwrite(&new_record, sizeof(ProductRecord), 1, fp);
		header.head_index = new_record_index;
		header.sequential_sorted = 0;
		fseek(fp, 0, SEEK_SET);
		fwrite(&header, sizeof(Header), 1, fp);
		fclose(fp);
		printf("\nRegistro inserido.\n");
		return 0;
	}

	fseek(fp, sizeof(Header) + (lower_index - 1) *sizeof(ProductRecord), SEEK_SET);
	ProductRecord low_record;
	fread(&low_record, sizeof(ProductRecord), 1, fp);

	if (low_record.product_id == new_record.product_id)
	{
		if (low_record.ativo == 1)
		{
			fclose(fp);
			printf("\nRegistro existente.\n");
			return 0;
		}

		fseek(fp, sizeof(Header) + (lower_index - 1) *sizeof(ProductRecord), SEEK_SET);
		new_record.seq_key = low_record.seq_key;
		new_record.elo = low_record.elo;
		low_record = new_record;
		fwrite(&low_record, sizeof(ProductRecord), 1, fp);
		fclose(fp);
		printf("\nRegistro inserido.\n");
		return 0;
	}

	new_record.elo = low_record.seq_key + 1;

	if (low_record.elo != 0)
		new_record.elo = low_record.elo;

	low_record.elo = new_record_index;

	long long low_record_seq_key = low_record.seq_key + 1;

	if (new_record_index == low_record_seq_key)
	{
		low_record.elo = 0;
	}
	else
	{
		header.sequential_sorted = 0;
	}

	fseek(fp, sizeof(Header) + (new_record_index - 1) *sizeof(ProductRecord), SEEK_SET);
	fwrite(&new_record, sizeof(ProductRecord), 1, fp);
	fseek(fp, sizeof(Header) + (lower_index - 1) *sizeof(ProductRecord), SEEK_SET);
	fwrite(&low_record, sizeof(ProductRecord), 1, fp);
	fseek(fp, 0, SEEK_SET);
	fwrite(&header, sizeof(Header), 1, fp);
	printf("\nRegistro inserido.\n");
	fclose(fp);
	return 0;
}

void display_records_via_elo(long long pag)
{
	FILE *fp = fopen(ORIGINAL_FILE_NAME, "rb");

	fseek(fp, 0, SEEK_END);
	long long file_size = ftell(fp);
	long long num_records = (file_size - sizeof(Header)) / sizeof(ProductRecord);

	if (pag < 1 || (pag - 1) *RECORDS_PER_PAGE >= num_records)
	{
		printf("\nEntrada invalida.\n");
		fclose(fp);
		return;
	}

	fseek(fp, 0, SEEK_SET);
	Header header;
	fread(&header, sizeof(Header), 1, fp);

	long long current_index = header.head_index;
	ProductRecord current_record;

	long long records_to_skip = (pag - 1) *RECORDS_PER_PAGE;

	long long skipped_records = 0;

	while (skipped_records < records_to_skip && current_index != -1)
	{
		fseek(fp, sizeof(Header) + (current_index - 1) *sizeof(ProductRecord), SEEK_SET);
		fread(&current_record, sizeof(ProductRecord), 1, fp);

		current_index++;

		if (current_record.elo != 0)
			current_index = current_record.elo;

		if (current_record.ativo)
		{
			skipped_records++;
		}
	}

	if (current_index == -1)
	{
		printf("\nNao ha registros nesta pagina.\n");
		fclose(fp);
		return;
	}

	printf("\nExibindo registros da pagina %lld seguindo os elos:\n", pag);
	long long records_displayed = 0;
	while (records_displayed < RECORDS_PER_PAGE && current_index != -1)
	{
		fseek(fp, sizeof(Header) + (current_index - 1) *sizeof(ProductRecord), SEEK_SET);
		fread(&current_record, sizeof(ProductRecord), 1, fp);

		if (current_record.ativo)
		{
			printf("\nRegistro %lld:\n", records_displayed + 1);
			printf("  Product ID: %lld\n", current_record.product_id);
			printf("  Category ID: %lld\n", current_record.category_id);
			printf("  Category Code: %s\n", current_record.category_code);
			printf("  Brand: %s\n", current_record.brand);
			printf("  Price: %.2f\n", current_record.price);
			printf("  Seq Key: %lld\n", current_record.seq_key);
			if (current_record.elo != 0)
				printf("  Elo (Proximo Indice): %lld\n", current_record.elo);

			records_displayed++;
		}

		current_index++;

		if (current_record.elo != 0)
			current_index = current_record.elo;
	}

	fclose(fp);
}

int create_sorted_file_chunks(const char *original_file, const char *sorted_file)
{
	printf("\nOrdenando sequencialmente o arquivo de dados...\n");
	FILE *fp_orig = fopen(original_file, "rb");
	Header header;
	fread(&header, sizeof(Header), 1, fp_orig);
	FILE *fp_sorted = fopen(sorted_file, "w+b");
	Header sorted_header;
	sorted_header.head_index = 1;
	sorted_header.sequential_sorted = 1;
	fwrite(&sorted_header, sizeof(Header), 1, fp_sorted);
	long long current_index = header.head_index;
	ProductRecord *buffer = (ProductRecord*) malloc(CHUNK_SIZE* sizeof(ProductRecord));
	int buffer_count = 0;
	int seq_count = 1;
	int registros_escritos = 0;

	while (current_index != -1)
	{
		fseek(fp_orig, sizeof(Header) + (current_index - 1) *sizeof(ProductRecord), SEEK_SET);
		fread(&buffer[buffer_count], sizeof(ProductRecord), 1, fp_orig);

		current_index++;

		if (buffer[buffer_count].elo || buffer[buffer_count].elo == -1)
			current_index = buffer[buffer_count].elo;

		if (buffer[buffer_count].ativo == 1)
		{
			buffer_count++;
			registros_escritos++;
		}

		if (buffer_count == CHUNK_SIZE)
		{
			for (int i = 0; i < buffer_count; i++)
			{
				buffer[i].seq_key = seq_count;
				buffer[i].elo = 0;
				fwrite(&buffer[i], sizeof(ProductRecord), 1, fp_sorted);
				seq_count++;
			}

			buffer_count = 0;
		}
	}

	if (buffer_count > 0)
	{
		for (int i = 0; i < buffer_count; i++)
		{
			if (buffer[buffer_count].ativo == 1)
			{
				buffer[i].seq_key = seq_count;
				buffer[i].elo = 0;
				fwrite(&buffer[i], sizeof(ProductRecord), 1, fp_sorted);
				seq_count++;
			}
		}
	}

	fflush(fp_sorted);
	long current_pos = ftell(fp_sorted);
	long long last_record_pos = current_pos - sizeof(ProductRecord);

	if (registros_escritos > 0)
	{
		fseek(fp_sorted, last_record_pos, SEEK_SET);
		ProductRecord last_record;
		fread(&last_record, sizeof(ProductRecord), 1, fp_sorted);
		last_record.elo = -1;
		fseek(fp_sorted, last_record_pos, SEEK_SET);
		fwrite(&last_record, sizeof(ProductRecord), 1, fp_sorted);
	}

	free(buffer);
	fclose(fp_orig);
	fclose(fp_sorted);
	return 0;
}

int replace_original_with_sorted(const char *original_file, const char *sorted_file)
{
	remove(original_file);
	rename(sorted_file, original_file);
	return 0;
}

int binary_search_product_id(long long target_product_id, ProductRecord *result)
{
	FILE *fp = fopen(ORIGINAL_FILE_NAME, "rb");
	Header header;
	fread(&header, sizeof(Header), 1, fp);
	fclose(fp);

	if (header.sequential_sorted == 0)
	{
		create_sorted_file_chunks(ORIGINAL_FILE_NAME, SORTED_FILE_NAME);
		replace_original_with_sorted(ORIGINAL_FILE_NAME, SORTED_FILE_NAME);
	}

	fp = fopen(ORIGINAL_FILE_NAME, "rb+");
	fseek(fp, 0, SEEK_END);
	long long file_size = ftell(fp);
	long long num_records = (file_size - sizeof(Header)) / sizeof(ProductRecord);
	long long left = 0;
	long long right = num_records - 1;
	long long mid;
	ProductRecord mid_record;

	while (left <= right)
	{
		mid = left + (right - left) / 2;
		fseek(fp, sizeof(Header) + mid* sizeof(ProductRecord), SEEK_SET);
		fread(&mid_record, sizeof(ProductRecord), 1, fp);
		if (mid_record.product_id == target_product_id)
		{*result = mid_record;
			fclose(fp);
			return mid;
		}
		else if (mid_record.product_id < target_product_id)
		{
			left = mid + 1;
		}
		else
		{
			if (mid == 0) break;
			right = mid - 1;
		}
	}

	fclose(fp);
	return -1;
}

void remove_record(long long target_product_id)
{
	ProductRecord found_record;
	long long found_index = binary_search_product_id(target_product_id, &found_record);

	if (found_record.ativo == 0 || found_index == -1)
	{
		printf("\nRegistro inexistente. Exclusao nao realizada.\n");
		return;
	}

	found_record.ativo = 0;
	FILE *fp = fopen(ORIGINAL_FILE_NAME, "rb+");
	fseek(fp, sizeof(Header) + found_index* sizeof(ProductRecord), SEEK_SET);
	fwrite(&found_record, sizeof(ProductRecord), 1, fp);
	printf("\nExclusao realizada.\n");
	fclose(fp);

}

void search_and_display_product(long long target_product_id)
{
	ProductRecord found_record;
	long long found_index = binary_search_product_id(target_product_id, &found_record);

	if (found_index != -1 && found_record.ativo)
	{
		printf("\nProduto encontrado no indice %lld:\n", found_index + 1);
		printf("  Product ID: %lld\n", found_record.product_id);
		printf("  Category ID: %lld\n", found_record.category_id);
		printf("  Category Code: %s\n", found_record.category_code);
		printf("  Brand: %s\n", found_record.brand);
		printf("  Price: %.2f\n", found_record.price);
		printf("  Seq Key: %lld\n", found_record.seq_key);
	}
	else
	{
		printf("\nProduto com product_id %lld nao encontrado.\n", target_product_id);
	}
}

void print_all_records_sequential(long long pag)
{
	FILE *fp = fopen(ORIGINAL_FILE_NAME, "rb");
	Header header;
	fread(&header, sizeof(Header), 1, fp);

	if (header.sequential_sorted == 0)
	{
		create_sorted_file_chunks(ORIGINAL_FILE_NAME, SORTED_FILE_NAME);
		replace_original_with_sorted(ORIGINAL_FILE_NAME, SORTED_FILE_NAME);
	}

	fseek(fp, 0, SEEK_END);
	long long file_size = ftell(fp);
	long long num_records = (file_size - sizeof(Header)) / sizeof(ProductRecord);

	if (pag < 1 || (pag - 1) *RECORDS_PER_PAGE >= num_records)
	{
		printf("\nEntrada invalida.\n");
		fclose(fp);
		return;
	}

	long long start_record = (pag - 1) *RECORDS_PER_PAGE;
	fseek(fp, sizeof(Header) + start_record* sizeof(ProductRecord), SEEK_SET);

	printf("\nExibindo registros da pagina %lld:\n", pag);
	ProductRecord record;
	for (long long i = 0; i < RECORDS_PER_PAGE && (start_record + i) < num_records; i++)
	{
		fread(&record, sizeof(ProductRecord), 1, fp);
		if (record.ativo)
		{
			printf("Registro %lld:\n", start_record + i + 1);
			printf("  Product ID: %lld\n", record.product_id);
			printf("  Category ID: %lld\n", record.category_id);
			printf("  Category Code: %s\n", record.category_code);
			printf("  Brand: %s\n", record.brand);
			printf("  Price: %.2f\n", record.price);
			printf("  Seq Key: %lld\n\n", record.seq_key);
		}
		else
		{
			i--;
		}
	}

	fclose(fp);
}

void build_partial_index_file()
{
	printf("\nConstruindo o indice parcial do arquivo de dados...\n");

	FILE *data_fp = fopen(ORIGINAL_FILE_NAME, "rb");
	FILE *index_fp = fopen(INDEX_FILE_NAME, "wb");

	Header header;
	fread(&header, sizeof(Header), 1, data_fp);

	fclose(data_fp);

	if (header.sequential_sorted == 0)
	{
		create_sorted_file_chunks(ORIGINAL_FILE_NAME, SORTED_FILE_NAME);
		replace_original_with_sorted(ORIGINAL_FILE_NAME, SORTED_FILE_NAME);
	}

	data_fp = fopen(ORIGINAL_FILE_NAME, "rb");
	fseek(data_fp, sizeof(Header), SEEK_SET);

	long long record_count = 0;
	ProductRecord record;

	while (fread(&record, sizeof(ProductRecord), 1, data_fp) == 1)
	{
		if (record_count % INDEX_INTERVAL == 0)
		{
			IndexRecord idx_record;
			idx_record.product_id = record.product_id;
			idx_record.seq_key = record.seq_key;
			fwrite(&idx_record, sizeof(IndexRecord), 1, index_fp);
		}

		record_count++;
	}

	fclose(data_fp);
	fclose(index_fp);

}

void search_and_display_product_via_index(long long target_product_id)
{
	FILE *data_fp = fopen(ORIGINAL_FILE_NAME, "rb");
	ProductRecord product_result;

	Header header;
	fread(&header, sizeof(Header), 1, data_fp);

	fclose(data_fp);

	FILE *index_fp = fopen(INDEX_FILE_NAME, "rb");

	if (index_fp == NULL || header.sequential_sorted == 0)
	{
		if (index_fp != NULL)
		{
			fclose(index_fp);
		}

		build_partial_index_file();
		index_fp = fopen(INDEX_FILE_NAME, "rb");
	}

	fseek(index_fp, 0, SEEK_END);
	long long file_size = ftell(index_fp);
	long long num_records = file_size / sizeof(IndexRecord);
	long long left = 0;
	long long right = num_records - 1;
	long long mid;
	IndexRecord index_record;

	while (left <= right)
	{
		mid = left + (right - left) / 2;
		fseek(index_fp, mid* sizeof(IndexRecord), SEEK_SET);
		fread(&index_record, sizeof(IndexRecord), 1, index_fp);
		if (index_record.product_id == target_product_id)
		{
			break;
		}
		else if (index_record.product_id < target_product_id)
		{
			if (mid == num_records - 1) break;
			left = mid + 1;
		}
		else
		{
			if (mid == 0) break;
			right = mid - 1;
		}
	}

	data_fp = fopen(ORIGINAL_FILE_NAME, "rb");

	if (index_record.product_id == target_product_id)
	{
		fseek(data_fp, sizeof(Header) + (index_record.seq_key - 1) *sizeof(ProductRecord), SEEK_SET);
		fread(&product_result, sizeof(ProductRecord), 1, data_fp);
	}
	else if (mid == num_records - 1)
	{
		fseek(data_fp, 0, SEEK_END);
		file_size = ftell(data_fp);
		num_records = (file_size - sizeof(Header)) / sizeof(ProductRecord);
		left = index_record.seq_key - 1;
		right = num_records - 1;
		ProductRecord product_record;
		while (left <= right)
		{
			mid = left + (right - left) / 2;
			fseek(data_fp, sizeof(Header) + mid* sizeof(ProductRecord), SEEK_SET);
			fread(&product_record, sizeof(ProductRecord), 1, data_fp);
			if (product_record.product_id == target_product_id)
			{
				product_result = product_record;
				break;
			}
			else if (product_record.product_id < target_product_id)
			{
				left = mid + 1;
			}
			else
			{
				if (mid == 0) break;
				right = mid - 1;
			}
		}
	}
	else if (mid != 0 || left != 0)
	{
		IndexRecord record;
		fseek(index_fp, left* sizeof(IndexRecord), SEEK_SET);
		fread(&record, sizeof(IndexRecord), 1, index_fp);
		left = right;
		right = record.seq_key - 1;
		fseek(index_fp, left* sizeof(IndexRecord), SEEK_SET);
		fread(&record, sizeof(IndexRecord), 1, index_fp);
		left = record.seq_key - 1;
		ProductRecord product_record;
		while (left <= right)
		{
			mid = left + (right - left) / 2;
			fseek(data_fp, sizeof(Header) + mid* sizeof(ProductRecord), SEEK_SET);
			fread(&product_record, sizeof(ProductRecord), 1, data_fp);
			if (product_record.product_id == target_product_id)
			{
				product_result = product_record;
				break;
			}
			else if (product_record.product_id < target_product_id)
			{
				if (mid == num_records - 1) break;
				left = mid + 1;
			}
			else
			{
				if (mid == 0) break;
				right = mid - 1;
			}
		}
	}

	if (product_result.ativo && product_result.product_id == target_product_id)
	{
		printf("\nProduto encontrado no indice %lld:\n", product_result.seq_key);
		printf("  Product ID: %lld\n", product_result.product_id);
		printf("  Category ID: %lld\n", product_result.category_id);
		printf("  Category Code: %s\n", product_result.category_code);
		printf("  Brand: %s\n", product_result.brand);
		printf("  Price: %.2f\n", product_result.price);
		printf("  Seq Key: %lld\n", product_result.seq_key);
	}
	else
	{
		printf("\nProduto com product_id %lld nao encontrado.\n", target_product_id);
	}

	fclose(data_fp);
	fclose(index_fp);

}

int compare_until_blankspace(const char *str1, const char *str2)
{
	while (*str1 && *str2 && *str1 == *str2 && *str1 != ' ')
	{
		str1++;
		str2++;
	}

	if ((*str1 == ' ' || *str1 == '\0') && (*str2 == ' ' || *str2 == '\0'))
	{
		return 0;
	}
	else
	{
		return (*str1 - *str2);
	}
}

void display_brand_records(const char *brand_name)
{
	char brand_file_name[256];
	snprintf(brand_file_name, sizeof(brand_file_name), FILE_ABSOLUTE_PATH "brand_%s.bin", brand_name);

	FILE *brand_fp = fopen(brand_file_name, "rb");

	ProductRecord records[RECORDS_PER_PAGE];
	int records_read;
	int page = 1;
	char user_input[10];

	do {
		records_read = fread(records, sizeof(ProductRecord), RECORDS_PER_PAGE, brand_fp);
		if (records_read == 0)
		{
			printf("\nNao existe ou nao existe mais registros com essa brand para exibir.\n");
			break;
		}

		printf("\nExibindo pagina %d:\n\n", page);
		for (int i = 0; i < records_read; i++)
		{
			ProductRecord *record = &records[i];
			printf("\nRegistro %d:\n", (page - 1) *RECORDS_PER_PAGE + i + 1);
			printf("  Product ID: %lld\n", record->product_id);
			printf("  Category ID: %lld\n", record->category_id);
			printf("  Category Code: %s\n", record->category_code);
			printf("  Brand: %s\n", record->brand);
			printf("  Price: %.2f\n", record->price);
			printf("  Seq Key: %lld\n\n", record->seq_key);
		}

		printf("Exibir a proxima pagina? (s/n): ");
		fgets(user_input, sizeof(user_input), stdin);

		size_t len = strlen(user_input);
		if (len > 0 && user_input[len - 1] == '\n')
		{
			user_input[len - 1] = '\0';
		}

		page++;
	} while (strcmp(user_input, "s") == 0 || strcmp(user_input, "s") == 0);

	fclose(brand_fp);
	remove(brand_file_name);

}

void create_brand_file(const char *brand_name)
{
	char brand_file_name[256];
	snprintf(brand_file_name, sizeof(brand_file_name), FILE_ABSOLUTE_PATH "brand_%s.bin", brand_name);

	FILE *orig_fp = fopen(ORIGINAL_FILE_NAME, "rb");
	FILE *brand_fp = fopen(brand_file_name, "wb");

	fseek(orig_fp, sizeof(Header), SEEK_SET);

	ProductRecord record;
	while (fread(&record, sizeof(ProductRecord), 1, orig_fp) == 1)
	{
		if (record.ativo == 1 && compare_until_blankspace(record.brand, brand_name) == 0)
		{
			fwrite(&record, sizeof(ProductRecord), 1, brand_fp);
		}
	}

	fclose(orig_fp);
	fclose(brand_fp);
}

void create_price_file(float min_price, float max_price)
{
	char price_file_name[] = FILE_ABSOLUTE_PATH "price_filtered.bin";

	FILE *orig_fp = fopen(ORIGINAL_FILE_NAME, "rb");

	FILE *price_fp = fopen(price_file_name, "wb");

	fseek(orig_fp, sizeof(Header), SEEK_SET);

	ProductRecord record;
	while (fread(&record, sizeof(ProductRecord), 1, orig_fp) == 1)
	{
		if (record.ativo)
		{
			int match = 1;

			if (record.price < min_price)
			{
				match = 0;
			}

			if (record.price > max_price && max_price != 0)
			{
				match = 0;
			}

			if (match)
			{
				fwrite(&record, sizeof(ProductRecord), 1, price_fp);
			}
		}
	}

	fclose(orig_fp);
	fclose(price_fp);
}

void display_price_records()
{
	char price_file_name[] = FILE_ABSOLUTE_PATH "price_filtered.bin";

	FILE *price_fp = fopen(price_file_name, "rb");;

	ProductRecord records[RECORDS_PER_PAGE];
	int records_read;
	int page = 1;
	char user_input[10];

	do {
		records_read = fread(records, sizeof(ProductRecord), RECORDS_PER_PAGE, price_fp);
		if (records_read == 0)
		{
			printf("\nNao existe ou nao existe mais registros com esse intervalo de valores para exibir.\n");
			break;
		}

		printf("\nExibindo pagina %d:\n", page);
		for (int i = 0; i < records_read; i++)
		{
			ProductRecord *record = &records[i];
			printf("\nRegistro %d:\n", (page - 1) *RECORDS_PER_PAGE + i + 1);
			printf("  Product ID: %lld\n", record->product_id);
			printf("  Category ID: %lld\n", record->category_id);
			printf("  Category Code: %s\n", record->category_code);
			printf("  Brand: %s\n", record->brand);
			printf("  Price: %.2f\n", record->price);
			printf("  Seq Key: %lld\n\n", record->seq_key);
		}

		printf("Exibir a proxima pagina? (s/n): ");
		fgets(user_input, sizeof(user_input), stdin);

		size_t len = strlen(user_input);
		if (len > 0 && user_input[len - 1] == '\n')
		{
			user_input[len - 1] = '\0';
		}

		page++;
	} while (strcmp(user_input, "s") == 0 || strcmp(user_input, "S") == 0);

	fclose(price_fp);
	remove(price_file_name);
}

int main()
{
	int choice;


	do {
		printf("\nMenu:\n");
		printf("1. Inserir um novo registro\n");
		printf("2. Exibir registros via elo (paginacao)\n");
		printf("3. Exibir registros sequencialmente (paginacao)\n");
		printf("4. Buscar um produto pelo product_id\n");
		printf("5. Remover um produto pelo product_id\n");
		printf("6. Buscar um produto via indice\n");
		printf("7. Exibir produtos por marca\n");
		printf("8. Exibir produtos por faixa de preco\n");
		printf("9. Sair\n");
		printf("\nDigite sua escolha: ");
		scanf("%d", &choice);
		getchar();

		switch (choice)
		{
			case 1:
				{
					long long product_id, category_id;
					char category_code[MAX_CATEGORY_CODE_LEN];
					char brand[MAX_BRAND_LEN];
					float price;
					printf("\nDigite product_id: ");
					scanf("%lld", &product_id);
					getchar();
					printf("\nDigite category_id: ");
					scanf("%lld", &category_id);
					getchar();
					printf("\nDigite category_code: ");
					fgets(category_code, MAX_CATEGORY_CODE_LEN, stdin);
					category_code[strcspn(category_code, "\n")] = '\0';

					printf("\nDigite brand: ");
					fgets(brand, MAX_BRAND_LEN, stdin);
					brand[strcspn(brand, "\n")] = '\0';

					printf("\nDigite price: ");
					scanf("%f", &price);
					getchar();
					ProductRecord new_product = create_sample_product(product_id, category_id, category_code, brand, price);
					insert_record(new_product);
				}

				break;
			case 2:
				{
					long long page;
					printf("\nDigite o numero da pagina: ");
					scanf("%lld", &page);
					getchar();
					display_records_via_elo(page);
				}

				break;
			case 3:
				{
					long long page;
					printf("\nDigite o numero da pagina: ");
					scanf("%lld", &page);
					getchar();
					print_all_records_sequential(page);
				}

				break;
			case 4:
				{
					struct timespec start, end;
					double duration;
					long long product_id;
					printf("\nDigite o product_id para buscar: ");
					scanf("%lld", &product_id);
					getchar();
					if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
                        fprintf(stderr, "Erro ao obter o tempo inicial: %s\n", strerror(errno));
                        break;
                    }

					search_and_display_product(product_id);

					if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
                        fprintf(stderr, "Erro ao obter o tempo final: %s\n", strerror(errno));
                        break;
                    }

                    duration = (end.tv_sec - start.tv_sec) + 
                               (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("Operacao selecao 'product_id' pelo arquivo de dados executada em %.6f segundos.\n", duration);
				}

				break;
			case 5:
				{
					long long product_id;
					printf("\nDigite o product_id para remover: ");
					scanf("%lld", &product_id);
					getchar();
					remove_record(product_id);
				}

				break;
			case 6:
				{
					struct timespec start, end;
					double duration;
					long long product_id;
					printf("\nDigite o product_id para buscar via indice: ");
					scanf("%lld", &product_id);
					getchar();
					if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
                        fprintf(stderr, "Erro ao obter o tempo inicial: %s\n", strerror(errno));
                        break;
                    }
					search_and_display_product_via_index(product_id);
					if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
                        fprintf(stderr, "Erro ao obter o tempo final: %s\n", strerror(errno));
                        break;
                    }

                    duration = (end.tv_sec - start.tv_sec) + 
                               (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("Operacao selecao 'product_id' pelo arquivo de indice executada em %.6f segundos.\n", duration);
				}

				break;
			case 7:
				{
					struct timespec start, end;
					double duration;
					char brand_name[MAX_BRAND_LEN];
					printf("\nDigite o nome da brand: ");
					fgets(brand_name, MAX_BRAND_LEN, stdin);
					brand_name[strcspn(brand_name, "\n")] = '\0';
					if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
                        fprintf(stderr, "Erro ao obter o tempo inicial: %s\n", strerror(errno));
                        break;
                    }
					create_brand_file(brand_name);
					if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
                        fprintf(stderr, "Erro ao obter o tempo final: %s\n", strerror(errno));
                        break;
                    }
					duration = (end.tv_sec - start.tv_sec) + 
                               (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("Operacao selecao 'brand' executada em %.6f segundos.\n", duration);
					display_brand_records(brand_name);
				}

				break;
			case 8:
				{
					struct timespec start, end;
					double duration;
					float min_price = 0.0;
					float max_price = 0.0;
					int option = 0;

					printf("\nSelecione a opcao de filtro de preco:\n");
					printf("1. Ate um certo preco (<= X)\n");
					printf("2. A partir de um certo preco (>= X)\n");
					printf("3. Entre dois precos (X <= preco <= Y)\n");
					printf("Digite sua escolha (1-3): ");
					scanf("%d", &option);
					getchar();

					if (option == 1)
					{
						printf("Digite o preco maximo (X): ");
						scanf("%f", &max_price);
						getchar();
						min_price = 0.0;
					}
					else if (option == 2)
					{
						printf("Digite o preco minimo (X): ");
						scanf("%f", &min_price);
						getchar();
						max_price = 0.0;
					}
					else if (option == 3)
					{
						printf("Digite o preco minimo (X): ");
						scanf("%f", &min_price);
						getchar();
						printf("Digite o preco maximo (Y): ");
						scanf("%f", &max_price);
						getchar();
					}
					else
					{
						printf("Opcao invalida selecionada.\n");
						break;
					}

				    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
                        fprintf(stderr, "Erro ao obter o tempo inicial: %s\n", strerror(errno));
                        break;
                    }
					create_price_file(min_price, max_price);

					if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
                        fprintf(stderr, "Erro ao obter o tempo final: %s\n", strerror(errno));
                        break;
                    }
					duration = (end.tv_sec - start.tv_sec) + 
                               (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("Operacao selecao 'price' executada em %.6f segundos.\n", duration);
					display_price_records();
				}

				break;
			case 9:
				printf("\nSaindo...\n");
				break;
			default:
				printf("\nEscolha invalida.\n");
				break;
		}
	} while (choice != 9);

	return 0;
}
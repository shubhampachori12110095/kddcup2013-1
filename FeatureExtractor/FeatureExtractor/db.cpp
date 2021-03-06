#include "db.h"

#define BUFFER_SIZE 8192

bool authorCompare(const Author *a, const Author *b) { return a->id < b->id; }
bool paperCompare(const Paper *a, const Paper *b) { return a->id < b->id; }
bool paperAuthorCompare(const PaperAuthor *a, const PaperAuthor *b) { return a->paper_id < b->paper_id; }
bool paperAuthorIndexCompare(const PaperAuthorIndex *a, const PaperAuthorIndex *b) { return a->author_id < b->author_id; }
bool conferenceCompare(const Conference *a, const Conference *b) { return a->id < b->id; }
bool JournalCompare(const Journal *a, const Journal *b) { return a->id < b->id; }
bool isPrintable(char c) { return c >= 32 && c <= 126; }

inline char my_getc(FILE *fp, char buffer[BUFFER_SIZE], int &buffer_count, int &buffer_p){
	if (buffer_p >= buffer_count){
		buffer_count = fread(buffer, sizeof(char), BUFFER_SIZE, fp);
		buffer_p = 0;
		return (buffer_count == 0) ? EOF : buffer[buffer_p++];
	}
	else {
		return buffer[buffer_p++];
	}
}

int getIdFromBuffer(FILE *fp, char buffer[BUFFER_SIZE], int &buffer_count, int &buffer_p)
{
	int id = 0;
	char c = my_getc(fp, buffer, buffer_count, buffer_p);
	if (c == EOF) return -1;
	while (c != ',' && c != EOF){
		id = id * 10 + (c - '0');
		c = my_getc(fp, buffer, buffer_count, buffer_p);
	}
	if (c == EOF) return -1;
	return id;
}

FILE *getFile(char *datapath, char *filename)
{
	fprintf(stderr, "parse%s... ", filename);
	char buf[128];
	errno_t err;

	// {filename}.csv
	sprintf_s(buf, "%s/%s.csv", datapath, filename);
	FILE *fp;
	if ((err = fopen_s(&fp, buf, "r")) != 0){
		fprintf(stderr, "Error occured while loading %s.csv!\n", filename);
		exit(1);
	}
	// Skip head line
	fgets(buf, 128, fp);
	return fp;
}

void parseAuthor(DB *db)
{
	clock_t start_time = std::clock();
	FILE *fp = getFile(db->datapath, "Author");
	char buffer[BUFFER_SIZE];
	int buffer_count = 0;
	int buffer_p = 0;

	while (true){
		Author author;
		author.id = getIdFromBuffer(fp, buffer, buffer_count, buffer_p);
		if (author.id == -1) break;

		char c = 0;
		char name[144], affiliation[144];
		int name_index = 0, affiliation_index = 0;
		int state = 0; // 0: name, 1: affiliation
		bool quote = false;

		while (true) {
			c = my_getc(fp, buffer, buffer_count, buffer_p);
			if (!quote && c == ','){
				if (state == 0){
					state = 1;
				}
				else {
					fprintf(stderr, "Parsing error - %d!\n", author.id);
					exit(1);
				}
			}
			else if (c == '\"'){
				quote = !quote;
			}
			else if (isPrintable(c)) { //printable ascii
				if (state == 0){
					name[name_index++] = c;
				}
				else {
					affiliation[affiliation_index++] = c;
				}
			}
			else if ((!quote && c == '\n') || c == EOF){
				break;
			}
		}
		name[name_index] = 0;
		affiliation[affiliation_index] = 0;
		author.name = std::string(name);
		author.affiliation = std::string(affiliation);

		db->authors.push_back(new Author(author));
		if (c == EOF) {
			break;
		}
	}
	sort(db->authors.begin(), db->authors.end(), authorCompare);
	fprintf(stderr, "ok (%d ms)\n", std::clock() - start_time);
	fclose(fp);
}

void parsePaper(DB *db)
{
	clock_t start_time = std::clock();
	FILE *fp = getFile(db->datapath, "Paper");
	char buffer[BUFFER_SIZE];
	int buffer_count = 0;
	int buffer_p = 0;

	while (true){
		Paper paper;
		paper.id = getIdFromBuffer(fp, buffer, buffer_count, buffer_p);
		if (paper.id == -1) break;

		char c = 0;
		char title[144], keywords[144];
		int title_index = 0, keyword_index = 0;
		int state = 0; // 0: title, 1: year, 2: ConferenceId, 3: JournalId, 4: Keyword
		bool quote = false;
		paper.year = 0;
		paper.conference_id = 0;
		paper.journal_id = 0;

		while (true) {
			c = my_getc(fp, buffer, buffer_count, buffer_p);
			if (!quote && c == ','){
				if (state < 4){
					state++;
				}
				else {
					fprintf(stderr, "Parsing error - %d!\n", paper.id);
					exit(1);
				}
			}
			else if (c == '\"'){
				quote = !quote;
			}
			else if (isPrintable(c)) { //printable ascii
				if (state == 0){ // title
					title[title_index++] = c;
				}
				else if (state == 1){ // year
					if (c >= '0' && c <= '9'){
						paper.year = paper.year * 10 + (c - '0');
					}
				}
				else if (state == 2){ // conference id
					if (c >= '0' && c <= '9'){
						paper.conference_id = paper.conference_id * 10 + (c - '0');
					}
				}
				else if (state == 3){ // journal id
					if (c >= '0' && c <= '9'){
						paper.journal_id = paper.journal_id * 10 + (c - '0');
					}
				}
				else if (state == 4){
					keywords[keyword_index++] = c;
				}
			}
			else if ((!quote && c == '\n') || c == EOF){
				break;
			}
		}
		title[title_index] = 0;
		keywords[keyword_index] = 0;
		paper.title = std::string(title);
		paper.keywords = std::string(keywords);
		db->papers.push_back(new Paper(paper));
		if (c == EOF) {
			break;
		}
	}
	std::sort(db->papers.begin(), db->papers.end(), paperCompare);
	fprintf(stderr, "ok (%d ms)\n", std::clock() - start_time);
	fclose(fp);
}

void parsePaperAuthor(DB *db)
{
	clock_t start_time = std::clock();
	FILE *fp = getFile(db->datapath, "PaperAuthor");
	char buffer[BUFFER_SIZE];
	int buffer_count = 0;
	int buffer_p = 0;

	while (true){
		PaperAuthor paper_author;
		paper_author.paper_id = getIdFromBuffer(fp, buffer, buffer_count, buffer_p);
		paper_author.author_id = getIdFromBuffer(fp, buffer, buffer_count, buffer_p);
		if (paper_author.paper_id == -1 || paper_author.author_id == -1) break;

		char c = 0;
		char name[144], affiliation[144];
		int name_index = 0, affiliation_index = 0;
		int state = 0; // 0: name, 1: affiliation
		bool quote = false;

		while (true) {
			c = my_getc(fp, buffer, buffer_count, buffer_p);
			if (!quote && c == ','){
				if (state == 0){
					state = 1;
				}
				else {
					fprintf(stderr, "Parsing error - %d!\n", paper_author.paper_id);
					exit(1);
				}
			}
			else if (c == '\"'){
				quote = !quote;
			}
			else if (isPrintable(c)) { //printable ascii
				if (state == 0){
					name[name_index++] = c;
				}
				else {
					affiliation[affiliation_index++] = c;
				}
			}
			else if ((!quote && c == '\n') || c == EOF){
				break;
			}
		}
		name[name_index] = 0;
		affiliation[affiliation_index] = 0;
		paper_author.name = std::string(name);
		paper_author.affiliation = std::string(affiliation);
		db->paper_authors.push_back(new PaperAuthor(paper_author));
		if (c == EOF) {
			break;
		}
	}
	std::sort(db->paper_authors.begin(), db->paper_authors.end(), paperAuthorCompare);

	// make index...
	PaperAuthorIndex paper_author_index;
	for (size_t i = 0; i < db->paper_authors.size(); i++){
		paper_author_index.author_id = db->paper_authors[i]->author_id;
		paper_author_index.paper_author_index = i;
		db->paper_author_index.push_back(new PaperAuthorIndex(paper_author_index));
	}
	std::sort(db->paper_author_index.begin(), db->paper_author_index.end(), paperAuthorIndexCompare);

	fprintf(stderr, "ok (%d ms)\n", std::clock() - start_time);
	fclose(fp);
}

void parseConference(DB *db)
{
	clock_t start_time = std::clock();
	FILE *fp = getFile(db->datapath, "Conference");
	char buffer[BUFFER_SIZE];
	int buffer_count = 0;
	int buffer_p = 0;

	while (true){
		Conference conference;
		conference.id = getIdFromBuffer(fp, buffer, buffer_count, buffer_p);
		if (conference.id == -1) break;

		char c = 0;
		char shortname[144], fullname[144], homepage[144];
		int shortname_index = 0, fullname_index = 0, homepage_index = 0;
		int state = 0; // 0: shortname, 1: fullname, 2: homepage
		bool quote = false;

		while (true) {
			c = my_getc(fp, buffer, buffer_count, buffer_p);
			if (!quote && c == ','){
				if (state < 2){
					state++;
				}
				else {
					fprintf(stderr, "Parsing error - %d!\n", conference.id);
					exit(1);
				}
			}
			else if (c == '\"'){
				quote = !quote;
			}
			else if (isPrintable(c)) { //printable ascii
				if (state == 0){ // shortname
					shortname[shortname_index++] = c;
				}
				else if (state == 1){ // fullname
					fullname[fullname_index++] = c;
				}
				else if (state == 2){ // homepage
					homepage[homepage_index++] = c;
				}
			}
			else if ((!quote && c == '\n') || c == EOF){
				break;
			}
		}
		shortname[shortname_index] = 0;
		fullname[fullname_index] = 0;
		homepage[homepage_index] = 0;
		conference.shortname = std::string(shortname);
		conference.fullname = std::string(fullname);
		conference.homepage = std::string(homepage);
		db->conferences.push_back(new Conference(conference));
		if (c == EOF) {
			break;
		}
	}
	std::sort(db->conferences.begin(), db->conferences.end(), conferenceCompare);
	fprintf(stderr, "ok (%d ms)\n", std::clock() - start_time);
	fclose(fp);
}

void parseConferenceCluster(DB *db)
{
	clock_t start_time = std::clock();
	FILE *fp = getFile(db->datapath, "ConferenceCluster");

	while (!feof(fp)){
		int conference_id, cluster;
		fscanf_s(fp, "%d,%d", &conference_id, &cluster);
		if (feof(fp)) break;
		Conference *conference = db->getConferenceById(conference_id);
		if (conference != NULL){
			conference->cluster = cluster;
		}
	}
	fprintf(stderr, "ok (%d ms)\n", std::clock() - start_time);
	fclose(fp);
}

void parseJournal(DB *db)
{
	clock_t start_time = std::clock();
	FILE *fp = getFile(db->datapath, "Journal");
	char buffer[BUFFER_SIZE];
	int buffer_count = 0;
	int buffer_p = 0;

	while (true){
		Journal journal;
		journal.id = getIdFromBuffer(fp, buffer, buffer_count, buffer_p);
		if (journal.id == -1) break;

		char c = 0;
		char shortname[144], fullname[144], homepage[144];
		int shortname_index = 0, fullname_index = 0, homepage_index = 0;
		int state = 0; // 0: shortname, 1: fullname, 2: homepage
		bool quote = false;

		while (true) {
			c = my_getc(fp, buffer, buffer_count, buffer_p);
			if (!quote && c == ','){
				if (state < 2){
					state++;
				}
				else {
					fprintf(stderr, "Parsing error - %d!\n", journal.id);
					exit(1);
				}
			}
			else if (c == '\"'){
				quote = !quote;
			}
			else if (isPrintable(c)) { //printable ascii
				if (state == 0){ // shortname
					shortname[shortname_index++] = c;
				}
				else if (state == 1){ // fullname
					fullname[fullname_index++] = c;
				}
				else if (state == 2){ // homepage
					homepage[homepage_index++] = c;
				}
			}
			else if ((!quote && c == '\n') || c == EOF){
				break;
			}
		}
		shortname[shortname_index] = 0;
		fullname[fullname_index] = 0;
		homepage[homepage_index] = 0;
		journal.shortname = std::string(shortname);
		journal.fullname = std::string(fullname);
		journal.homepage = std::string(homepage);
		db->journals.push_back(new Journal(journal));
		if (c == EOF) {
			break;
		}
	}
	std::sort(db->journals.begin(), db->journals.end(), JournalCompare);
	fprintf(stderr, "ok (%d ms)\n", std::clock() - start_time);
	fclose(fp);
}

void parseJournalCluster(DB *db)
{
	clock_t start_time = std::clock();
	FILE *fp = getFile(db->datapath, "JournalCluster");

	while (!feof(fp)){
		int journal_id, cluster;
		fscanf_s(fp, "%d,%d", &journal_id, &cluster);
		if (feof(fp)) break;
		Journal *journal = db->getJournalById(journal_id);
		if (journal != NULL){
			journal->cluster = cluster;
		}
	}
	fprintf(stderr, "ok (%d ms)\n", std::clock() - start_time);
	fclose(fp);
}

DB *loadDB(char *datapath)
{
	DB *db = new DB();
	strcpy_s(db->datapath, datapath);

	parseConference(db);
	parseConferenceCluster(db);
	parseJournal(db);
	parseJournalCluster(db);
	parseAuthor(db);
	parsePaper(db);
	parsePaperAuthor(db);

	return db;
}

Author* DB::getAuthorById(int id)
{
	// do binary search
	int left = 0;
	int right = authors.size() - 1;

	while (left <= right){
		int mid = (left + right) / 2;
		if (authors[mid]->id == id){
			return authors[mid];
		}
		else if (authors[mid]->id < id){
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}
	return NULL;
}

Paper* DB::getPaperById(int id)
{
	// do binary search
	int left = 0;
	int right = papers.size() - 1;

	while (left <= right){
		int mid = (left + right) / 2;
		if (papers[mid]->id == id){
			return papers[mid];
		}
		else if (papers[mid]->id < id){
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}
	return NULL;
}

Conference* DB::getConferenceById(int id)
{
	// do binary search
	int left = 0;
	int right = conferences.size() - 1;

	while (left <= right){
		int mid = (left + right) / 2;
		if (conferences[mid]->id == id){
			return conferences[mid];
		}
		else if (conferences[mid]->id < id){
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}
	return NULL;
}

Journal* DB::getJournalById(int id)
{
	// do binary search
	int left = 0;
	int right = journals.size() - 1;

	while (left <= right){
		int mid = (left + right) / 2;
		if (journals[mid]->id == id){
			return journals[mid];
		}
		else if (journals[mid]->id < id){
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}
	return NULL;
}

void DB::getPaperAuthorsByPaperId(std::vector<PaperAuthor*> &result, int paper_id)
{
	// do binary search
	int left = 0;
	int right = paper_authors.size() - 1;

	int start = paper_authors.size();
	while (left <= right){
		int mid = (left + right) / 2;
		if (paper_authors[mid]->paper_id == paper_id){
			start = mid;
			break;
		}
		else if (paper_authors[mid]->paper_id < paper_id){
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}
	while (start > 0 && paper_authors[start-1]->paper_id == paper_id) start--;

	// Put the paper authors into result
	result.clear();
	for (size_t i = start; i < paper_authors.size() && paper_authors[i]->paper_id == paper_id; i++){
		result.push_back(paper_authors[i]);
	}
}

void DB::getPaperAuthorsByAuthorId(std::vector<PaperAuthor*> &result, int author_id)
{
	// do binary search
	int left = 0;
	int right = paper_author_index.size() - 1;

	int start = paper_author_index.size();
	while (left <= right){
		int mid = (left + right) / 2;
		if (paper_author_index[mid]->author_id == author_id){
			start = mid;
			break;
		}
		else if (paper_author_index[mid]->author_id < author_id){
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}
	while (start > 0 && paper_author_index[start - 1]->author_id == author_id) start--;

	// Put the paper_authors into result
	result.clear();
	for (size_t i = start; i < paper_author_index.size() && paper_author_index[i]->author_id == author_id; i++){
		result.push_back(paper_authors[ paper_author_index[i]->paper_author_index ]);
	}
	std::sort(result.begin(), result.end(), paperAuthorCompare);
}

void DB::getPaperAuthorsById(std::vector<PaperAuthor*> &result, int paper_id, int author_id)
{
	result.clear();
	std::vector<PaperAuthor*> author_papers;
	getPaperAuthorsByPaperId(author_papers, paper_id);
	for (size_t i = 0; i < author_papers.size(); i++){
		if (author_papers[i]->author_id == author_id){
			result.push_back(author_papers[i]);
		}
	}
}
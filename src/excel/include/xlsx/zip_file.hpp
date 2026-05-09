#pragma once

#include "duckdb/common/typedefs.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {

class ClientContext;

class ZipFileReader;

enum class ZipOpenMode { Create, Append };

class ZipFileWriter {
public:
	// Default: truncates any existing file at file_name and writes a fresh zip.
	ZipFileWriter(ClientContext &context, const string &file_name)
	    : ZipFileWriter(context, file_name, ZipOpenMode::Create) {
	}
	// Append mode: opens an existing zip in place; new entries are written after the existing
	// central directory. The central directory is rebuilt on close, so duplicate-name entries
	// (e.g. a replacement xl/workbook.xml) are resolved last-wins by readers.
	ZipFileWriter(ClientContext &context, const string &file_name, ZipOpenMode mode);
	~ZipFileWriter();

	// Delete copy
	ZipFileWriter(const ZipFileWriter &) = delete;
	ZipFileWriter &operator=(const ZipFileWriter &) = delete;

	void AddDirectory(const string &dir_name);
	void BeginFile(const string &file_name);
	idx_t Write(const char *buffer, idx_t write_size);
	idx_t Write(const string &str);
	idx_t Write(const char *str);

	void EndFile();
	void Finalize();

	// Copy a single entry (file or directory) verbatim from a source archive into this one.
	// The source must not have an entry currently open.
	void CopyEntryFrom(ZipFileReader &source, const string &entry_name);

private:
	void *handle;
	void *stream;
	bool is_entry_open;
	vector<char> escaped_buffer;
};

class ZipFileReader {
public:
	ZipFileReader(ClientContext &context, const string &file_name);
	~ZipFileReader();

	// Delete copy
	ZipFileReader(const ZipFileReader &) = delete;
	ZipFileReader &operator=(const ZipFileReader &) = delete;

	bool TryOpenEntry(const string &file_name);
	void CloseEntry();
	idx_t Read(char *buffer, idx_t read_size);

	// Returns the current position in the current entry
	idx_t GetEntryPos() const;
	// Returns the uncompressed size of the current entry
	idx_t GetEntryLen() const;
	// Returns if the current entry is done
	bool IsDone() const;

	// Walk all entries in the archive and return their names. The reader must not have an entry currently open.
	vector<string> ListEntries();

	// Returns true if the entry exists and is a directory entry (filename ends with '/').
	bool EntryIsDirectory(const string &entry_name);

private:
	friend class ZipFileWriter;

	void *handle;
	void *stream;
	bool is_entry_open;

	idx_t entry_pos;
	idx_t entry_len;
};

} // namespace duckdb
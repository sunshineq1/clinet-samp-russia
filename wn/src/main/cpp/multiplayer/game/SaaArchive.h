#pragma once

#include <cstdint>

// Архивы .saa из кеша Black Russia. Внутри обычный ZIP, у которого подпись
// локального заголовка заменена на DA BC 85 74, центрального каталога нет,
// а записи лежат без сжатия — поэтому файл внутри читается прямым смещением.
class CSaaArchive {
public:
    // Ищет в корне игры все *.saa и разбирает их оглавление
    static void MountAll();

    static bool Mount(const char* relativePath);

    // relativePath — как его просит игра, регистр и слэши любые.
    // При успехе отдаёт путь к архиву и границы записи внутри него.
    static bool Find(const char* relativePath, const char*& archive,
                     uint32_t& offset, uint32_t& size);

    static int32_t ArchiveCount();
    static int32_t EntryCount();

    // OS_FileSize для файлов через stdio не смотрит на границы окна и отдаёт
    // размер всего архива. Поэтому окна, которые открыли мы, помним сами.
    static void NoteWindow(void *handle, uint32_t size);
    static bool WindowSize(void *handle, uint32_t &size);
    static void ForgetWindow(void *handle);
};

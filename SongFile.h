/****************************************************************************************
*
*   Объявление класса SongFile
*
*	Объект этого класса представляет собой файл с песней.
*
*   Авторы: Людмила Огордникова и Александр Огородников, 2008-2010
*
****************************************************************************************/

class SongFile
{
	// указатель на объект класса MidiFile
	MidiFile *m_pMidiFile;

public:

	SongFile();
	~SongFile();

	// устанавливает кодовую страницу по умолчанию для слов песни
	bool SetDefaultCodePage(
		__in UINT DefaultCodePage);

	// устанавливает критерий выбора ноты из созвучия
	bool SetConcordNoteChoice(
		__in CONCORD_NOTE_CHOICE ConcordNoteChoice);

	// загружает файл с песней
	bool LoadFile(
		__in LPCTSTR pszFileName,
		__in UINT DefaultCodePage,
		__in CONCORD_NOTE_CHOICE ConcordNoteChoice);

	// создаёт объект класса Song, представляющий собой песню
	Song *CreateSong(
		__in DWORD QuantizeStepDenominator);

	// освобождает все выделенные ресурсы
	void Free();
};

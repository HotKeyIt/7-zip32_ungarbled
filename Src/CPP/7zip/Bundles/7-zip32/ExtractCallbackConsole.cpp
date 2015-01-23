// ExtractCallbackConsole.cpp

#include "StdAfx.h"

// #undef sprintf

#include "Dialog.h"											// �ǉ�
#include "resource.h"										// �ǉ�

#include "../../UI/Console/ConsoleClose.h"					//	�p�X�ύX
#include "ExtractCallbackConsole.h"							//	�p�X�ύX
//#include "UserInputUtils.h"								//	�폜

#include "../../../Common/IntToString.h"
#include "../../../Common/Wildcard.h"

#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/TimeUtils.h"
#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/PropVariantConv.h"

#include "../../Common/FilePathAutoRename.h"

#include "../../UI/Common/ExtractingFilePath.h"				//	�p�X�ύX

using namespace NWindows;
using namespace NFile;
using namespace NDir;

static const char *kTestString    =  "Testing     ";
static const char *kExtractString =  "Extracting  ";
static const char *kSkipString    =  "Skipping    ";

// static const char *kCantAutoRename = "can not create file with auto name\n";
// static const char *kCantRenameFile = "can not rename existing file\n";
// static const char *kCantDeleteOutputFile = "can not delete output file ";
static const char *kError = "ERROR: ";
static const char *kMemoryExceptionMessage = "Can't allocate required memory!";

static const char *kProcessing = "Processing archive: ";
static const char *kEverythingIsOk = "Everything is Ok";
static const char *kNoFiles = "No files to process";

static const char *kUnsupportedMethod = "Unsupported Method";
static const char *kCrcFailed = "CRC Failed";
static const char *kCrcFailedEncrypted = "CRC Failed in encrypted file. Wrong password?";
static const char *kDataError = "Data Error";
static const char *kDataErrorEncrypted = "Data Error in encrypted file. Wrong password?";
static const char *kUnavailableData = "Unavailable data";
static const char *kUnexpectedEnd = "Unexpected end of data";
static const char *kDataAfterEnd = "There are some data after the end of the payload data";
static const char *kIsNotArc = "Is not archive";
static const char *kHeadersError = "Headers Error";

static const char *k_ErrorFlagsMessages[] =
{
    "Is not archive"
  , "Headers Error"
  , "Headers Error in encrypted archive. Wrong password?"
  , "Unavailable start of archive"
  , "Unconfirmed start of archive"
  , "Unexpected end of archive"
  , "There are data after the end of archive"
  , "Unsupported method"
  , "Unsupported feature"
  , "Data Error"
  , "CRC Error"
};


STDMETHODIMP CExtractCallbackConsole::SetTotal(UInt64 size)		// �ύX
{
  if (g_StdOut.GetProgressDialog()) g_StdOut.GetProgressDialog()->SetTotal(size);					// �ǉ�
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::SetCompleted(const UInt64 *completeValue)		// �ύX
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;

  if (completeValue && g_StdOut.GetProgressDialog())				// �ǉ�
    g_StdOut.GetProgressDialog()->SetCompleted(*completeValue);		// �ǉ�
  return S_OK;
}

/* �폜��������
STDMETHODIMP CExtractCallbackConsole::AskOverwrite(
    const wchar_t *existName, const FILETIME *, const UInt64 *,
    const wchar_t *newName, const FILETIME *, const UInt64 *,
    Int32 *answer)
{
  (*OutStream) << "file " << existName << endl <<
        "already exists. Overwrite with" << endl <<
        newName;
  
  NUserAnswerMode::EEnum overwriteAnswer = ScanUserYesNoAllQuit(OutStream);
  
  switch (overwriteAnswer)
  {
    case NUserAnswerMode::kQuit:  return E_ABORT;
    case NUserAnswerMode::kNo:     *answer = NOverwriteAnswer::kNo; break;
    case NUserAnswerMode::kNoAll:  *answer = NOverwriteAnswer::kNoToAll; break;
    case NUserAnswerMode::kYesAll: *answer = NOverwriteAnswer::kYesToAll; break;
    case NUserAnswerMode::kYes:    *answer = NOverwriteAnswer::kYes; break;
    case NUserAnswerMode::kAutoRenameAll: *answer = NOverwriteAnswer::kAutoRename; break;
    default: return E_FAIL;
  }
  return S_OK;
}
   �폜�����܂� */

/* �ǉ��������� */
STDMETHODIMP CExtractCallbackConsole::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,	// �ύX
    const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,			// �ύX
    Int32 *answer)
{
  if (g_StdOut.GetProgressDialog() == NULL)
    return S_OK;
  CConfirmationDialog dlg;
  dlg.SetFileInfo(newName, *newSize, *newTime, existName, *existSize, *existTime);
  switch(dlg.DoModal())
  {
    case IDCANCEL:        return E_ABORT;
    case IDB_NO:          *answer = NOverwriteAnswer::kNo; break;
    case IDB_NO_ALL:      *answer = NOverwriteAnswer::kNoToAll; break;
    case IDB_YES_ALL:     *answer = NOverwriteAnswer::kYesToAll; break;
    case IDB_YES:         *answer = NOverwriteAnswer::kYes; break;
    case IDB_AUTO_RENAME: *answer = NOverwriteAnswer::kAutoRename; break;
    default: return E_FAIL;
  }
  return S_OK;
}
/* �ǉ������܂� */

STDMETHODIMP CExtractCallbackConsole::PrepareOperation(const wchar_t *name, bool /* isFolder */, Int32 askExtractMode, const UInt64 *position)
{
  const char *s;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract: s = kExtractString; break;
    case NArchive::NExtract::NAskMode::kTest:    s = kTestString; break;
    case NArchive::NExtract::NAskMode::kSkip:    s = kSkipString; break;
    default: s = ""; // return E_FAIL;
  };
  (*OutStream) << s << name;
  if (position != 0)
    (*OutStream) << " <" << *position << ">";
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::MessageError(const wchar_t *message)
{
  (*OutStream) << message << endl;
  NumFileErrorsInCurrent++;
  NumFileErrors++;
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::SetOperationResult(Int32 operationResult, bool encrypted)
{
  switch (operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      NumFileErrorsInCurrent++;
      NumFileErrors++;
      (*OutStream) << "  :  ";
      const char *s = NULL;
      switch (operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
          s = kUnsupportedMethod;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          s = (encrypted ? kCrcFailedEncrypted : kCrcFailed);
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          s = (encrypted ? kDataErrorEncrypted : kDataError);
          break;
        case NArchive::NExtract::NOperationResult::kUnavailable:
          s = kUnavailableData;
          break;
        case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
          s = kUnexpectedEnd;
          break;
        case NArchive::NExtract::NOperationResult::kDataAfterEnd:
          s = kDataAfterEnd;
          break;
        case NArchive::NExtract::NOperationResult::kIsNotArc:
          s = kIsNotArc;
          break;
        case NArchive::NExtract::NOperationResult::kHeadersError:
          s = kHeadersError;
          break;
      }
      /* �ǉ��������� */
      if (encrypted)
      {
        (*OutStream) << endl;
        return ERROR_PASSWORD_FILE;
      }
      /* �ǉ������܂� */
      if (s)
        (*OutStream) << "Error : " << s;
      else
      {
        char temp[16];
        ConvertUInt32ToString(operationResult, temp);
        (*OutStream) << "Error #" << temp;
      }
    }
  }
  (*OutStream) << endl;
  return S_OK;
}

#ifndef _NO_CRYPTO

HRESULT CExtractCallbackConsole::SetPassword(const UString &password)
{
  PasswordIsDefined = true;
  Password = password;
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    RINOK(GetPassword(Password, IDS_DECRYPT));		// �ύX
    PasswordIsDefined = true;
  }
  return StringToBstr(Password, password);
}

#endif

HRESULT CExtractCallbackConsole::BeforeOpen(const wchar_t *name)
{
  NumTryArcs++;
  ThereIsErrorInCurrent = false;
  ThereIsWarningInCurrent = false;
  NumFileErrorsInCurrent = 0;
  (*OutStream) << endl << kProcessing << name << endl;
  if (g_StdOut.GetProgressDialog()) g_StdOut.GetProgressDialog()->SetArchiveFile(name);	// �ǉ�
  return S_OK;
}

HRESULT CExtractCallbackConsole::OpenResult(const wchar_t * /* name */, HRESULT result, bool encrypted)
{
  (*OutStream) << endl;
  if (result != S_OK)
  {
    (*OutStream) << "Error: ";
    if (result == S_FALSE)
    {
      (*OutStream) << (encrypted ?
        "Can not open encrypted archive. Wrong password?" :
        "Can not open file as archive");
      if (encrypted)						// �ǉ�
        return ERROR_PASSWORD_FILE;			// �ǉ�
    }
    else
    {
      if (result == E_OUTOFMEMORY)
        (*OutStream) << "Can't allocate required memory";
      else
        (*OutStream) << NError::MyFormatMessage(result);
    }
    (*OutStream) << endl;
    NumCantOpenArcs++;
    ThereIsErrorInCurrent = true;
  }
  return S_OK;
}

AString GetOpenArcErrorMessage(UInt32 errorFlags)
{
  AString s;
  for (unsigned i = 0; i < ARRAY_SIZE(k_ErrorFlagsMessages); i++)
  {
    UInt32 f = (1 << i);
    if ((errorFlags & f) == 0)
      continue;
    const char *m = k_ErrorFlagsMessages[i];
    if (!s.IsEmpty())
      s += '\n';
    s += m;
    errorFlags &= ~f;
  }
  if (errorFlags != 0)
  {
    char sz[16];
    sz[0] = '0';
    sz[1] = 'x';
    ConvertUInt32ToHex(errorFlags, sz + 2);
    if (!s.IsEmpty())
      s += '\n';
    s += sz;
  }
  return s;
}


HRESULT CExtractCallbackConsole::SetError(int level, const wchar_t *name,
    UInt32 errorFlags, const wchar_t *errors,
    UInt32 warningFlags, const wchar_t *warnings)
{
  if (level != 0)
  {
    (*OutStream) << name << endl;
  }

  if (errorFlags != 0)
  {
    (*OutStream) << "Errors: ";
    (*OutStream) << endl;
    (*OutStream) << GetOpenArcErrorMessage(errorFlags);
    (*OutStream) << endl;
    NumOpenArcErrors++;
    ThereIsErrorInCurrent = true;
  }

  if (errors && wcslen(errors) != 0)
  {
    (*OutStream) << "Errors: ";
    (*OutStream) << endl;
    (*OutStream) << errors;
    (*OutStream) << endl;
    NumOpenArcErrors++;
    ThereIsErrorInCurrent = true;
  }

  if (warningFlags != 0)
  {
    (*OutStream) << "Warnings: ";
    (*OutStream) << endl;
    (*OutStream) << GetOpenArcErrorMessage(warningFlags);
    (*OutStream) << endl;
    NumOpenArcWarnings++;
    ThereIsWarningInCurrent = true;
  }

  if (warnings && wcslen(warnings) != 0)
  {
    (*OutStream) << "Warnings: ";
    (*OutStream) << endl;
    (*OutStream) << warnings;
    (*OutStream) << endl;
    NumOpenArcWarnings++;
    ThereIsWarningInCurrent = true;
  }

  (*OutStream) << endl;
  return S_OK;
}
  
HRESULT CExtractCallbackConsole::ThereAreNoFiles()
{
  (*OutStream) << endl << kNoFiles << endl;
  return S_OK;
}

HRESULT CExtractCallbackConsole::ExtractResult(HRESULT result)
{
  /* �ǉ��������� */
  CProgressDialog* pDlg = g_StdOut.GetProgressDialog();
  if (pDlg)
  {
    pDlg->SetCompleted(pDlg->GetTotalSize());
    pDlg->SendExtractingInfo(ARCEXTRACT_END);
  }
  /* �ǉ������܂� */
  if (result == S_OK)
  {
    (*OutStream) << endl;

    if (NumFileErrorsInCurrent == 0 && !ThereIsErrorInCurrent)
    {
      if (ThereIsWarningInCurrent)
        NumArcsWithWarnings++;
      else
        NumOkArcs++;
      (*OutStream) << kEverythingIsOk << endl;
    }
    else
    {
      NumArcsWithError++;
      if (NumFileErrorsInCurrent != 0)
        (*OutStream) << "Sub items Errors: " << NumFileErrorsInCurrent << endl;
    }
    return S_OK;
  }
  
//  NumArcsWithError++;										// �폜
//  if (result == E_ABORT || result == ERROR_DISK_FULL)		// �폜
//    return result;										// �폜
  /* �ǉ��������� */
  if (result == E_ABORT || result == ERROR_PASSWORD_FILE)
  {
    NumArcsWithError = 0;
	NumFileErrors = 0;
    return result;
  }
  NumArcsWithError++;
  if (result == ERROR_DISK_FULL)
	  return ERROR_DISK_FULL;
  /* �ǉ������܂� */
  (*OutStream) << endl << kError;
  if (result == E_OUTOFMEMORY)
    (*OutStream) << kMemoryExceptionMessage;
  else
    (*OutStream) << NError::MyFormatMessage(result);
  (*OutStream) << endl;
  return S_OK;
}

HRESULT CExtractCallbackConsole::OpenTypeWarning(const wchar_t *name, const wchar_t *okType, const wchar_t *errorType)
{
  UString s = L"Warning:\n";
  if (wcscmp(okType, errorType) == 0)
  {
    s += L"The archive is open with offset";
  }
  else
  {
    s += name;
    s += L"\nCan not open the file as [";
    s += errorType;
    s += L"] archive\n";
    s += L"The file is open as [";
    s += okType;
    s += L"] archive";
  }
 (*OutStream) << s << endl << endl;
 ThereIsWarningInCurrent = true;
  return S_OK;
}

#include "history.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#if MLN_UNIT_TESTS
#include "extern/doctest.h"
#endif

//-------------------------------------------------------------------------------------------------

#define HIST_LOG(msg, ...)  \
    do{ if(gLogHistory){    \
        printf("history: " msg "\n" __VA_OPT__(,) __VA_ARGS__);   \
    }}while(0)

//-------------------------------------------------------------------------------------------------

// the line history buffer is in two parts:
//  1. a rolling buffer of chars excluding newlines and terminators; just a big block of chars
//  2. ringbuffer of indices into the above + lengths
constexpr int kHistoryBufSize = 2 * 1024;
constexpr int kHistoryMaxLines = 32;

struct HistoryLine
{
    uint16_t StartIx = 0;
    uint16_t Length = 0;    // excluding trailing nul
    bool     Valid = false;
    uint8_t  Gen = 0;
};

static char gHistoryBuf[kHistoryBufSize] = {0};
static int gNextHistoryWriteChar = 0;
static HistoryLine gHistoryLines[kHistoryMaxLines];
static int gCurrHistoryLine = 0;
static uint8_t gCurrHistoryGeneration = 0;

static bool gHistoryDirty = false;
static bool gLogHistory = false;

//-------------------------------------------------------------------------------------------------

void hist_init()
{
    gNextHistoryWriteChar = 0;
    gCurrHistoryLine = 0;
    gCurrHistoryGeneration = 0;

    gHistoryDirty = false;

    for (HistoryLine& line : gHistoryLines)
        line.Valid = false;
}

//-------------------------------------------------------------------------------------------------

void hist_enable_logging(bool enable)
{
    gLogHistory = enable;
}

//-------------------------------------------------------------------------------------------------

static HistoryLine& get_line_to_write()
{
    if (gHistoryLines[gCurrHistoryLine].Valid)
    {
        ++gCurrHistoryLine;
        if (gCurrHistoryLine >= kHistoryMaxLines)
        {
            gCurrHistoryLine = 0;
            ++gCurrHistoryGeneration;
        }
    }

    return gHistoryLines[gCurrHistoryLine];
}

//-------------------------------------------------------------------------------------------------

// end_ix is exclusive
static void invalidate_lines_using_region(int start_ix, int end_ix)
{
    for (HistoryLine& line : gHistoryLines)
    {
        if (!line.Valid)
            continue;
        if ((line.StartIx >= end_ix) || ((line.StartIx + line.Length) < start_ix))
            continue;

        line.Valid = false;
    }
}

//-------------------------------------------------------------------------------------------------

// ensure that there are num_bytes contiguous free space to write to at gNextHistoryWriteChar
static bool alloc_line_space(int num_bytes)
{
    if (num_bytes > kHistoryBufSize)
        return false;

    // are we at the end of the buffer and need to wrap around?
    if (kHistoryBufSize < (gNextHistoryWriteChar + num_bytes))
    {
        invalidate_lines_using_region(gNextHistoryWriteChar, kHistoryBufSize);
        gNextHistoryWriteChar = 0;
    }

    invalidate_lines_using_region(gNextHistoryWriteChar, gNextHistoryWriteChar + num_bytes);

    return true;
}

//-------------------------------------------------------------------------------------------------

void hist_add_line(const char* line)
{
    if (!line)
        return;

    hist_jump_to_newest();

    // don't add whatever we already had
    if (!strcmp(line, hist_get_curr_line()))
        return;

    const int len = strlen(line);
    if (!alloc_line_space(len + 1))
        return;

    strcpy(gHistoryBuf + gNextHistoryWriteChar, line);

    HistoryLine& line_info = get_line_to_write();
    line_info.StartIx = gNextHistoryWriteChar;
    line_info.Length = len;
    line_info.Gen = gCurrHistoryGeneration;
    line_info.Valid = true;

    gNextHistoryWriteChar += len + 1;

    gHistoryDirty = true;

    HIST_LOG("adding line '%s'; now have %d lines", line, hist_count_lines());
}

//-------------------------------------------------------------------------------------------------

const char* hist_get_curr_line()
{
    const HistoryLine& line = gHistoryLines[gCurrHistoryLine];
    if (!line.Valid)
        return "";

    return gHistoryBuf + line.StartIx;
}

int hist_count_lines()
{
    int num_lines = 0;
    
    for (HistoryLine& line : gHistoryLines)
    {
        if (line.Valid)
            ++num_lines;
    }

    return num_lines;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

bool hist_prev()
{
    const HistoryLine& cline = gHistoryLines[gCurrHistoryLine];
    if (!cline.Valid)
    {
        HIST_LOG("prev: current is invalid");
        return false;
    }

    int prev_ix = gCurrHistoryLine - 1;
    if (prev_ix < 0)
        prev_ix = kHistoryMaxLines - 1;

    const HistoryLine& pline = gHistoryLines[prev_ix];
    if (!pline.Valid)
    {
        HIST_LOG("prev: prev is invalid");
        return false;
    }

    // generation check - so we can detect if we've rolled back so far that we've reached the most recent line again
    const uint8_t dgen = cline.Gen - pline.Gen;
    if (dgen > 1)
    {
        HIST_LOG("prev: prev is gen %d, we are %d; ignoring", int(pline.Gen), int(cline.Gen));
        return false;
    }

    HIST_LOG("prev: moving to prev line, %d -> %d", int(gCurrHistoryLine), int(prev_ix));
    gCurrHistoryLine = prev_ix;
    return true;
}

//-------------------------------------------------------------------------------------------------

bool hist_next()
{
    const HistoryLine& cline = gHistoryLines[gCurrHistoryLine];
    if (!cline.Valid)
    {
        HIST_LOG("next: current is invalid");
        return false;
    }

    uint8_t cline_gen = cline.Gen;

    int next_ix = gCurrHistoryLine + 1;
    if (next_ix >= kHistoryMaxLines)
    {
        HIST_LOG("next: wrapping past end; changing gen %d -> %d", cline_gen, int(cline_gen+1));
        next_ix = 0;
        ++cline_gen;
    }

    const HistoryLine& nline = gHistoryLines[next_ix];
    if (!nline.Valid)
    {
        HIST_LOG("next: next is invalid");
        return false;
    }

    // generation check - so we can detect if we've gone forward so far that we've reached the oldest entry again
    const uint8_t dgen = nline.Gen - cline_gen;
    if (dgen > 1)
    {
        HIST_LOG("next: next is gen %d, we are %d; ignoring", int(nline.Gen), int(cline_gen));
        return false;
    }

    HIST_LOG("next: moving to next line, %d -> %d", int(gCurrHistoryLine), int(next_ix));
    gCurrHistoryLine = next_ix;
    return true;
}

//-------------------------------------------------------------------------------------------------

void hist_jump_to_oldest()
{
    for (;;)
    {
        if (!hist_prev())
            break;
    }
}

void hist_jump_to_newest()
{
    for (;;)
    {
        if (!hist_next())
            break;
    }
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#if MLN_UNIT_TESTS
#include <string>


TEST_SUITE_BEGIN("history");


TEST_CASE("init")
{
    REQUIRE_EQ(hist_count_lines(), 0);
    hist_init();

    CHECK_EQ(hist_count_lines(), 0);
}


TEST_CASE("adding")
{
    hist_init();

    std::string s("line 1");

    hist_add_line(s.c_str());
    CHECK_EQ(hist_count_lines(), 1);
    CHECK_EQ(s, hist_get_curr_line());

    // make sure we don't add dupes
    hist_add_line(s.c_str());
    CHECK_EQ(hist_count_lines(), 1);
    CHECK_EQ(s, hist_get_curr_line());
}


TEST_CASE("skipping")
{
    hist_init();

    CHECK_EQ(hist_prev(), false);
    CHECK_EQ(hist_next(), false);

    const std::string lines[] { "first", "second", "third\nand\nhas\t\tweirdness", "fourth", "FIIIIIIIIIIFTH" };
    const int num_lines = sizeof(lines) / sizeof(lines[0]);
    for (const std::string& line : lines)
    {
        hist_add_line(line.c_str());
        CHECK_EQ(hist_next(), false);
    }

    CHECK_EQ(hist_count_lines(), num_lines);

    for (int line_ix = num_lines-1; line_ix > 0; --line_ix)
    {
        CHECK_EQ(lines[line_ix], std::string(hist_get_curr_line()));
        const bool more_to_go = hist_prev();
        CHECK_EQ(more_to_go, line_ix > 0);
    }

    CHECK_EQ(hist_prev(), false);   // we should have unwound the whole way by now
    CHECK_EQ(lines[0], hist_get_curr_line());

    for (int line_ix = 1; line_ix < num_lines; ++line_ix)
    {
        const bool more_to_go = hist_next();

        CHECK_EQ(more_to_go, true);
        CHECK_EQ(lines[line_ix], hist_get_curr_line());
    }

    CHECK_EQ(hist_next(), false);
}


TEST_CASE("line wrapping")
{
    hist_init();

    // make sure we can reach our max
    for (int i=0; i<kHistoryMaxLines; ++i)
    {
        CHECK_EQ(hist_count_lines(), i);

        hist_add_line(i&1 ? "x" : "X");
    }
    CHECK_EQ(hist_count_lines(), kHistoryMaxLines);

    // make sure we behave right after that
    for (int i=0; i<200; ++i)
    {
        hist_add_line(i&1 ? "y" : "Y");
        CHECK_EQ(hist_count_lines(), kHistoryMaxLines);
    }
}


TEST_CASE("buffer wrapping")
{
    hist_init();

    std::string longstr("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ");
    for (int i=0; i<100; ++i)
    {
        hist_add_line(longstr.c_str());
        CHECK_EQ(longstr, hist_get_curr_line());

        ++longstr[0];
        if (longstr[0] > 'z')
            longstr[0] = '0';
    }
}


TEST_CASE("skipping & wrapping")
{
    hist_init();

    const std::string lines[] { "aaaaaaaaaa", "bbbbbbbbb", "ccccccccccc", "dddddddddddddd", "eeeeeeeeeee", "ffffffffffff" };
    const int num_lines = sizeof(lines) / sizeof(lines[0]);
    for (int i = 0; i<100; ++i)
    {
        for (const std::string& line : lines)
        {
            hist_add_line(line.c_str());
            CHECK_EQ(hist_next(), false);
        }
    }

    CHECK_EQ(hist_count_lines(), kHistoryMaxLines);

    int line_ix = num_lines-1;
    for (;;)
    {
        if (!hist_prev())
            break;

        line_ix = (line_ix - 1 + num_lines) % num_lines;

        CHECK_EQ(lines[line_ix], std::string(hist_get_curr_line()));
    }
}


TEST_CASE("skipping & wrapping, generational")
{
    hist_init();
    gCurrHistoryGeneration = 253;

    const std::string lines[] { "aaaaaaaaaa", "bbbbbbbbb", "ccccccccccc", "dddddddddddddd", "eeeeeeeeeee", "ffffffffffff" };
    const int num_lines = sizeof(lines) / sizeof(lines[0]);
    for (int i = 0; i<1000; ++i)
    {
        for (const std::string& line : lines)
        {
            hist_add_line(line.c_str());
            CHECK_EQ(hist_next(), false);
        }

        // make sure we've thoroughly wrapped generations
        if (gCurrHistoryGeneration == 10)
            break;
    }

    CHECK_EQ(hist_count_lines(), kHistoryMaxLines);

    int line_ix = num_lines-1;
    for (;;)
    {
        if (!hist_prev())
            break;

        line_ix = (line_ix - 1 + num_lines) % num_lines;

        CHECK_EQ(lines[line_ix], std::string(hist_get_curr_line()));
    }
}



TEST_SUITE_END();
#endif

//-------------------------------------------------------------------------------------------------


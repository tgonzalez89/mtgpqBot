#ifndef REDIRECTSTREAM_H
#define REDIRECTSTREAM_H

#include <iostream>
#include <streambuf>
#include <string>
#include <sstream>
#include <fstream>
#include <QPlainTextEdit>
#include <QString>
#include <QObject>

class LogWriter : public QObject
{
    Q_OBJECT

public:
    LogWriter(std::string file) : log_file(file) {}

    void appendTextEdit(QString qs) {
        emit appendTextEdit_sig(qs);

        if (log_file == "") return;
        std::ofstream log_file_stream(log_file, std::ios_base::out | std::ios_base::app);
        if (!log_file_stream.is_open()) {
            std::cout << "-E- LogWriter::appendTextEdit - Can't open file: '" << log_file << "'" << std::endl;
            return;
        }
        log_file_stream << qs.toStdString() << std::endl;
        log_file_stream.close();
    }

signals:
    void appendTextEdit_sig(QString);

private:
    std::string log_file;
};

class RedirectStream : public std::basic_streambuf<char>
{
public:
RedirectStream(std::ostream &stream, QPlainTextEdit* text_edit, std::string file = "") :
    m_stream(stream),
    log_window(text_edit),
    log_file(file) {
    emitter = new LogWriter(log_file);
    QObject::connect(emitter, SIGNAL(appendTextEdit_sig(QString)), text_edit, SLOT(appendPlainText(QString)), Qt::QueuedConnection);
    m_old_buf = stream.rdbuf();
    stream.rdbuf(this);
}

~RedirectStream() {
    // Output anything that's left.
    if (!m_string.empty()) emitter->appendTextEdit(m_string.c_str());
    m_stream.rdbuf(m_old_buf);
    delete emitter;
}

protected:
virtual int_type overflow(int_type v) {
    if (v == '\n') {
        emitter->appendTextEdit(m_string.c_str());
        m_string.erase(m_string.begin(), m_string.end());
    } else {
        m_string += v;
    }
    return v;
}

virtual std::streamsize xsputn(const char* p, std::streamsize n) {
    m_string.append(p, p + n);
    int pos = 0;
    while (pos != std::string::npos) {
        pos = (int)m_string.find('\n');
        if (pos != std::string::npos) {
            std::string tmp(m_string.begin(), m_string.begin() + pos);
            emitter->appendTextEdit(tmp.c_str());
            m_string.erase(m_string.begin(), m_string.begin() + pos + 1);
        }
    }
    return n;
}

private:
    std::ostream &m_stream;
    std::streambuf* m_old_buf;
    std::string m_string;
    QPlainTextEdit* log_window;
    LogWriter* emitter;
    std::string log_file;
};

#endif // REDIRECTSTREAM_H

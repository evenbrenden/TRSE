#ifndef SYSTEMX86_H
#define SYSTEMX86_H


#include "abstractsystem.h"
#include <QProcess>
#include "source/LeLib/util/util.h"

class SystemX86 : public AbstractSystem
{
public:
    SystemX86(QSharedPointer<CIniFile> settings, QSharedPointer<CIniFile> proj) : AbstractSystem(settings, proj) {
        m_processor = PX86;
        m_system = X86;
        m_startAddress = 0x100;
        m_programStartAddress = m_startAddress;

        m_allowedGlobalTypeFlags << "compressed"<<"pure"<<"pure_variable" <<"pure_number" << "signed" <<"no_term";
        m_allowedProcedureTypeFlags << "pure"<<"pure_variable" <<"pure_number" << "signed" <<"no_term" <<"global";
        m_registers << "_ax"<<"_bx" <<"_cx" <<"_dx";
        m_registers << "_ah"<<"_bh" <<"_ch" <<"_dh";
        m_registers << "_al"<<"_bl" <<"_cl" <<"_dl";
//        m_registers << "_es"<<"_di" <<"_ds" <<"_si";
        m_registers << "_di" <<"_si";

    }

    virtual void Assemble(QString& text, QString file, QString currentDir, QSharedPointer<SymbolTable>  symTab);
    virtual void PostProcess(QString& text, QString file, QString currentDir);
    virtual bool is8bit() {
        return false;
    }
};


#endif // SYSTEMX86_H

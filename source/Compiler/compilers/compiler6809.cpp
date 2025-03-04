#include "compiler6809.h"

void Compiler6809::InitAssemblerAnddispatcher(QSharedPointer<AbstractSystem> system)
{
    m_codeGen = QSharedPointer<CodeGen6502>(new CodeGen6502());
    m_assembler = QSharedPointer<Asm6502>(new Asm6502());

    m_assembler->byte="fcb";
    m_assembler->word="fdb";

    m_assembler->m_zbyte = 0x10;

    Init6809Assembler();
    LabelStack::m_labelCount = 0;
}

void Compiler6809::Connect()
{
    m_assembler->Connect();


    if (m_ini->getdouble("post_optimize")==1.0) {
        emit EmitTick("<br>Optimising pass: ");
        m_assembler->m_totalOptimizedLines = 0;
        for (int i=0;i<4;i++) {
            emit EmitTick(" ["+QString::number(i+1)+"]");
            m_assembler->Optimise(*m_projectIni); }
    }


    CleanupBlockLinenumbers();
/*    for (QString&s : m_assembler->m_source) {
        s = s.replace("sta(","sta (");
        s = s.replace("lda(","lda (");
    }
    */
}

void Compiler6809::CleanupCycleLinenumbers(QString currentFile, QMap<int, int> &ocycles, QMap<int, int> &retcycles, bool isCycles)
{
}

void Compiler6809::Init6809Assembler()
{
    m_assembler->m_startInsertAssembler << m_parser.m_initAssembler;
    m_assembler->m_defines = m_parser.m_preprocessorDefines;


    if (Syntax::s.m_currentSystem->m_system == AbstractSystem::ATARI2600)
        m_assembler->IncludeFile(":resources/code/atari2600/init.asm");



    m_assembler->InitZeroPointers(m_projectIni->getStringList("zeropages"),m_projectIni->getStringList("temp_zeropages"),m_projectIni->getStringList("var_zeropages"));
    m_assembler->m_zeropageScreenMemory = m_projectIni->getString("zeropage_screenmemory");
    m_assembler->m_zeropageColorMemory = m_projectIni->getString("zeropage_colormemory");
    m_assembler->m_replaceValues["@DECRUNCH_ZP1"] = m_projectIni->getString("zeropage_decrunch1");
    m_assembler->m_replaceValues["@DECRUNCH_ZP2"] = m_projectIni->getString("zeropage_decrunch2");
    m_assembler->m_replaceValues["@DECRUNCH_ZP3"] = m_projectIni->getString("zeropage_decrunch3");
    m_assembler->m_replaceValues["@DECRUNCH_ZP4"] = m_projectIni->getString("zeropage_decrunch4");

/*    qDebug() << m_projectIni->contains("ignore_initial_jump");
    for (CItem i : m_projectIni->items)
        qDebug() << i.name;
*/
    m_assembler->m_ignoreInitialJump = m_projectIni->getdouble("ignore_initial_jump")==1.0;



    if (Syntax::s.m_currentSystem->m_system==AbstractSystem::VIC20) {
    QStringList lst = m_projectIni->getStringList("via_zeropages");
    if (lst.count()<4)
        ErrorHandler::e.Error("VIC-20 compilation error: You need to specify 4 1-byte VIA zero page values in the project settings.",0);
    m_assembler->m_replaceValues["@VIA_ZP1"] = lst[0];
    m_assembler->m_replaceValues["@VIA_ZP2"] = lst[1];
    m_assembler->m_replaceValues["@VIA_ZP3"] = lst[2];
    m_assembler->m_replaceValues["@VIA_ZP4"] = lst[3];
    }



    m_assembler->m_internalZP =
            RegisterStack(QStringList()
                          <<m_projectIni->getString("zeropage_internal1")
                          <<m_projectIni->getString("zeropage_internal2")
                          <<m_projectIni->getString("zeropage_internal3")
                          <<m_projectIni->getString("zeropage_internal4"));


    /*    m_assembler->m_internalZP << m_projectIni->getString("zeropage_internal1");
        m_assembler->m_internalZP << m_projectIni->getString("zeropage_internal2");
        m_assembler->m_internalZP << m_projectIni->getString("zeropage_internal3");
        m_assembler->m_internalZP << m_projectIni->getString("zeropage_internal4");
    */


    if (m_projectIni->getdouble("override_target_settings")==1) {
        Syntax::s.m_currentSystem->m_startAddress = Util::NumberFromStringHex(m_projectIni->getString("override_target_settings_basic"));
        Syntax::s.m_ignoreSys = m_projectIni->getdouble("override_target_settings_sys")==1;
        Syntax::s.m_currentSystem->m_programStartAddress = Util::NumberFromStringHex(m_projectIni->getString("override_target_settings_org"));
        Syntax::s.m_currentSystem->m_stripPrg = m_projectIni->getdouble("override_target_settings_prg")==1;
        if (Syntax::s.m_ignoreSys)
            Syntax::s.m_currentSystem->m_startAddress = Syntax::s.m_currentSystem->m_programStartAddress;
    } else {
        Syntax::s.m_currentSystem->DefaultValues();
        Syntax::s.m_ignoreSys = Syntax::s.m_currentSystem->m_ignoreSys;
 //       Syntax::s.m_stripPrg = false;

    }
   /* if (Syntax::s.m_currentSystem->m_system==AbstractSystem::ATARI2600 ||
          Syntax::s.m_currentSystem->m_system==AbstractSystem::ATARI2600  ) {
        Syntax::s.m_ignoreSys = true;
        Syntax::s.m_stripPrg = true;

    }*/

/*    if (Syntax::s.m_currentSystem->m_system==AbstractSystem::MEGA65) {
        Syntax::s.m_ignoreSys = true;
        Syntax::s.m_stripPrg = false;

    }
*/

    if (Syntax::s.m_currentSystem->isCommodoreSystem() && !Syntax::s.m_ignoreSys)
        Syntax::s.m_currentSystem->m_startAddress = Syntax::s.m_currentSystem->getDefaultBasicAddress();

    Syntax::s.m_ignoreSys=true;
    Syntax::s.m_currentSystem->m_ignoreSys = true;;
}


int Compiler6809::FindEndSymbol(Orgasm &orgasm)
{
    //    QStringList output = QString(out).split("\n");
    for (QString s : orgasm.m_symbols.keys()) {
        if (s.toLower().contains("endsymbol")) {
            return orgasm.m_symbols[s];
        }
    }
    return 0;
}

QVector<int> Compiler6809::FindBlockEndSymbols(Orgasm &orgasm)
{
    QVector<int> m_blockEndSymbols;

    m_blockEndSymbols.clear();
    for (QString s : orgasm.m_symbols.keys()) {
        if (s.toLower().contains("endblock")) {
            QString spl = s;
            spl = spl.toLower().simplified().split("block")[1];
            //            bool ok;
            int i= orgasm.m_symbols[s];//.toInt(&ok, 16);
            //qDebug() << "FOUND endblock : " << s << Util::numToHex(i);
            m_blockEndSymbols.append(i);
        }
    }
    return m_blockEndSymbols;
}

void Compiler6809::ConnectBlockSymbols(QVector<int> &blockEndSymbols)
{
    for (int sym : blockEndSymbols) {
        int winner = 0xFFFF;
        QSharedPointer<MemoryBlock> winnerBlock=nullptr;

        for (QSharedPointer<MemoryBlock> mb: m_assembler->blocks) {
            //            if (mb->m_type==MemoryBlock::CODE &&  sym>mb->m_start)
            if (sym>mb->m_start)
                if (sym-mb->m_start<winner) {
                    winner = sym-mb->m_start;
                    winnerBlock  =mb;
                }
        }
        if (winnerBlock!=nullptr) {
            winnerBlock->m_end = sym;
            //     qDebug() << Util::numToHex(sym) << " " << winnerBlock->Type();
        }
    }


}

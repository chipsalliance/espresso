#!/usr/bin/env amm

interp.repositories() ++= Seq(
  coursierapi.MavenRepository.of(
    "https://oss.sonatype.org/content/repositories/snapshots"
  )
)

@
import $ivy.`edu.berkeley.cs::chisel3:3.5-SNAPSHOT`
import $ivy.`edu.berkeley.cs::chiseltest:0.5-SNAPSHOT`

import chisel3._
import chisel3.util._
import chisel3.util.experimental.decode._
import chiseltest.HasTestName
import chiseltest.formal.{BoundedCheck, Formal}
import logger.{LogLevel, LogLevelAnnotation}

class TestModule(unminimized: TruthTable, minimized: TruthTable)
    extends Module {
  val i = IO(Input(UInt(unminimized.table.head._1.getWidth.W)))
  val (unminimizedI, unminimizedO) = pla(unminimized.table.toSeq)
  val (minimizedI, minimizedO) = pla(minimized.table.toSeq)
  unminimizedI := i
  minimizedI := i

  chisel3.experimental.verification.assert(
    // compare UInt with BitPat, chisel will handle DC automatically.
    // TODO default
    (unminimized.table.map { case (input, output) =>
      (i === input) && (minimizedO === output)
    } ++ Seq(unminimized.table.keys.map(i =/= _).reduce(_ && _))).reduce(_ || _)
  )
}

class Test[T <: Module](name: String, dutGen: => T)
    extends Formal
    with HasTestName {
  override def getTestName = name
  def verify(): Unit =
    verify(
      dutGen,
      Seq(LogLevelAnnotation(LogLevel.Error), BoundedCheck(1))
    ) // TODO: kmax = 0
}

def readTable(fileName: os.Path): TruthTable = {
  def bitPat(espresso: String): BitPat = BitPat(
    "b" + espresso.replaceAll("-|0", "?") // TODO: has no meaning => DC ?
  )

  // if there is more than one row with the same input,
  // use this to merge their output
  def specialAdd(left: Char, right: Char): Char = {
    val zeroDash = Seq('0', '-')
    if (zeroDash.contains(left)) right // "has no meaning" & DC is not the same
    else if (zeroDash.contains(right)) left
    else if (left == right) left
    else 'x' // x is wrong!
  }

  val lines = os
    .read(fileName)
    .split('\n')
    .filterNot(_.startsWith("#"))
    .map(_.split('#').head)
  val inputLength =
    lines.filter(_.startsWith(".i")).head.split("\\s+")(1).toInt
  val outputLength =
    lines.filter(_.startsWith(".o")).head.split("\\s+")(1).toInt

  val inputOutput = lines
    .filterNot(_.startsWith("."))
    .mkString
    .replaceAll("\\s+|[|]", "")
    .replace('2', '-') // 2 == -, it can be used in {in,out}put
    .grouped(inputLength + outputLength)
    .toList
    .map(_.splitAt(inputLength))
    // in type fd, both ~ & 0 implies "has no meaning". ~ is only used in output
    .map(io => (io._1, io._2.replace("~", "0")))

  val table = inputOutput
    .groupBy(_._1)
    .map { case (str, tuples) => str -> tuples.map(_._2) }
    .map { case (str, strings) =>
      str -> strings.reduce(_.zip(_).map((specialAdd _).tupled).mkString)
    }
    .map { case (str, str1) => bitPat(str) -> bitPat(str1) }

  val default = BitPat(
    s"b${"0" * table.head._2.getWidth}"
  ) // TODO: infer from .type

  TruthTable(table, default)
}

val devNull = os.root / "dev" / "null"
os.walk(os.pwd / "examples")
  .filter(os.isFile)
  .filterNot("o64" == _.baseName)
  .foreach { inputFile =>
    var succ = true
    val outputFile = os.temp(prefix = inputFile.baseName)
    os.proc("espresso", s"$inputFile")
      .call(
        stdout = outputFile,
        stderr = devNull
      ) // stderr: span more than one line (warning)
    try {
      new Test(
        inputFile.baseName,
        new TestModule(readTable(inputFile), readTable(outputFile))
      ).verify()
    } catch {
      case e: Exception => succ = false
      case e: Error     => succ = false
    }
    println(s"${inputFile.baseName} ${if (succ) "S" else "F"}")
  }
// vg2 S
// squar5 S
// xor5 S
// misex2 F
// apex1 S
// clip S
// 9sym S
// Z9sym S
// rd73 S
// ex5 F
// cps F
// 5xp1 F
// misex3 F
// e64 S
// duke2 F
// seq F
// apex5 F
// b12 F
// table5 F
// t481 F
// mytest F
// alu4 F
// Z5xp1 F
// bw F
// apex2 S
// rd84 S
// apex4 F
// apex3 F
// rd53 S
// sao2 F
// con1 F
// misex1 F
// table3 F
// inc F
// cordic F
// spla S
// misex3c F
// misj F
// mainpla F
// test2 F
// xparc F
// soar F
// jbp F
// ex4 S
// test3 F
// mish F
// x7dn S
// ts10 S
// shift S
// ti S
// ibm S
// ex1010 F
// pdc S
// signet S
// x2dn F
// misg F
// alu3 F
// al2 F
// br1 S
// risc F
// x1dn S
// chkn S
// dekoder F
// wim F
// b3 F
// gary F
// max1024 F
// check S
// root S
// dc2 S
// m1 F
// m3 F
// newtpla S
// sqr6 S
// t4 F
// newapla2 S
// ryy6 F
// vtx1 S
// m2 F
// newtpla1 S
// max128 F
// exp S
// prom1 F
// dk17 S
// alcom F
// max512 F
// x9dn S
// dist S
// t3 F
// exps F
// tcheck F
// newxcpla1 F
// b7 F
// newtag F
// sqn S
// vg2 S
// amd S
// luc F
// newbyte S
// in3 F
// bcd F
// t2 S
// dk27 S
// mp2d F
// pope F
// mytest3 F
// opa S
// in2 S
// in7 F
// alu2 F
// prom2 S
// mlp4 S
// in4 F
// ex7 S
// f51m F
// in6 S
// newcpla1 F
// newtpla2 S
// m4 F
// x6dn S
// bcc F
// p82 F
// mark1 F
// newill S
// lin F
// in1 F
// bca F
// br2 S
// check~ S
// b10 F
// max46 S
// newcond S
// b2 F
// b4 S
// newapla F
// in5 S
// bcb F
// clpl S
// apla S
// dk48 S
// mytest F
// exep S
// bc0 F
// check2 S
// sex F
// in0 F
// mytest2 F
// t1 F
// newapla1 S
// alu1 F
// tms S
// newcwp F
// newcpla2 F
// dc1 F
// intb F

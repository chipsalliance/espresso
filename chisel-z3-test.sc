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

  // type fdr: 1/4 -> ON-set, 0 -> OFF-set, -/2 -> DC-set, ~/3 -> has no meaning
  def bitPat(espresso: String): BitPat = BitPat(
    "b" + espresso
      .replace('4', '1')
      .replaceAll("[-2]", "?")
      .replaceAll("[~3]", "?")
  )

  val table = os
    .read(fileName)
    .split('\n')
    .filterNot(_.startsWith("."))
    .map(_.split(' '))
    .map(arr => arr(0) -> arr(1))
    .map { case (in, out) => bitPat(in) -> bitPat(out) }

  val default = BitPat(
    s"b${"0" * table.head._2.getWidth}"
  ) // TODO: infer from .type

  TruthTable(table, default)
}

val devNull = os.root / "dev" / "null"
val res = os
  .walk(os.pwd / "examples")
  .filter(os.isFile)
  .filterNot("o64" == _.baseName)
  .map { origPla =>
    val fileName = origPla.last
    print(fileName)

    val origPlaFdr = os.temp(prefix = s"${fileName}_fdr")
    // convert original PLA to fdr type w/o minify it.
    os.proc("espresso", "-ofdr", "-Decho", s"$origPla")
      .call(stdout = origPlaFdr, stderr = devNull)

    val minimizedPla = os.temp(prefix = s"${fileName}_minimized")
    os.proc("espresso", "-ofdr", s"$origPlaFdr").call(stdout = minimizedPla)

    var succ = true
    try {
      new Test(
        fileName,
        new TestModule(readTable(origPlaFdr), readTable(minimizedPla))
      ).verify()
    } catch {
      case _: Throwable => succ = false
    }
    println(s" ${if (succ) "S" else "F"}")
    succ
  }
sys.exit(if (res.reduce(_ && _)) 0 else 250)

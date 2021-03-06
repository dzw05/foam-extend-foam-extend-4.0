    Info<< "Reading field p_rgh\n" << endl;
    volScalarField p_rgh
    (
        IOobject
        (
            "p_rgh",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    );

    Info<< "Reading field alpha1\n" << endl;
    volScalarField alpha1
    (
        IOobject
        (
            "alpha1",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    );

    Info<< "Reading field U\n" << endl;
    volVectorField U
    (
        IOobject
        (
            "U",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    );

    #include "createPhi.H"

    Info<< "Reading transportProperties\n" << endl;
    twoPhaseMixture twoPhaseProperties(U, phi);

    const dimensionedScalar& rho1 = twoPhaseProperties.rho1();
    const dimensionedScalar& rho2 = twoPhaseProperties.rho2();

    dimensionedScalar Dab(twoPhaseProperties.lookup("Dab"));

    // Read the reciprocal of the turbulent Schmidt number
    dimensionedScalar alphatab(twoPhaseProperties.lookup("alphatab"));

    // Need to store rho for ddt(rho, U)
    volScalarField rho("rho", alpha1*rho1 + (scalar(1) - alpha1)*rho2);
    rho.oldTime();


    // Mass flux
    // Initialisation does not matter because rhoPhi is reset after the
    // alpha1 solution before it is used in the U equation.
    surfaceScalarField rhoPhi
    (
        IOobject
        (
            "rho*phi",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        rho1*phi
    );

    Info<< "Calculating field (g.h)f\n" << endl;
    volScalarField gh("gh", g & mesh.C());
    surfaceScalarField ghf = surfaceScalarField("gh", g & mesh.Cf());

    volScalarField p
    (
        IOobject
        (
            "p",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        p_rgh + rho*gh
    );

    label p_rghRefCell = 0;
    scalar p_rghRefValue = 0.0;
    setRefCell(p_rgh, pimple.dict(), p_rghRefCell, p_rghRefValue);
    mesh.schemesDict().setFluxRequired(p_rgh.name());

    scalar pRefValue = 0.0;

    if (p_rgh.needReference())
    {
        pRefValue = readScalar(pimple.dict().lookup("pRefValue"));

        p += dimensionedScalar
        (
            "p",
            p.dimensions(),
            pRefValue - getRefCellValue(p, p_rghRefCell)
        );
    }


    // Construct incompressible turbulence model
    autoPtr<incompressible::turbulenceModel> turbulence
    (
        incompressible::turbulenceModel::New(U, phi, twoPhaseProperties)
    );

module utils_module

    !> The effective value of \f$ log(0) \f$
    double precision, parameter :: logzero = -huge(0d0)
    !> The effective value of \f$ log(\inf) \f$
    double precision, parameter :: loginf = +huge(0d0) 

    !> The maximum character length
    integer, parameter :: STR_LENGTH = 100

    !> \f$ 2\pi \f$ in double precision
    double precision, parameter :: TwoPi = 8d0*atan(1d0)

    !> The default double format
    !!
    !! should have write statements along the lines of 
    !! write(*,'(E<DBL_FMT(1)>.<DBL_FMT(2)>)')
    integer, parameter, dimension(2) :: DBL_FMT=(/17,8/)

    !> unit for stdout
    integer, parameter :: stdout_unit = 6
    !> unit for reading from resume file
    integer, parameter :: read_resume_unit = 10
    !> unit for writing to resum file
    integer, parameter :: write_resume_unit = 11
    !> unit for writing to txt file
    integer, parameter :: write_txt_unit = 12
    !> unit for writing dead file
    integer, parameter :: write_dead_unit = 13


    contains



    !> Euclidean distance of two coordinates
    !!
    !! returns \f$\sqrt{\sum_i (a_i-b_i)^2 } \f$
    function distance(a,b)
        implicit none
        !> First vector
        double precision, dimension(:) :: a
        !> Second vector
        double precision, dimension(:) :: b

        double precision :: distance

        distance = sqrt( dot_product(a-b,a-b) )

    end function distance


    !> Double comparison
    function dbleq(a,b)
        implicit none
        double precision :: a,b
        logical :: dbleq
        double precision, parameter :: eps = 1d-7

        dbleq =  abs(a-b) < eps * max(abs(a),abs(b)) 

    end function dbleq







    !> How to actually calculate sums from logs.
    !!
    !! i.e. if one has a set of logarithms \f$\{\log(L_i)\}\f$, how should one
    !! calculate \f$ \log(\sum_i L_i)\f$ without underflow?
    !!
    !! One does it with the 'log-sum-exp' trick, by subtracting off the maximum
    !! value so that at least the maxmimum value doesn't underflow, and then add
    !! it back on at the end:
    function logsumexp(vector)
        implicit none
        !> vector of log(w
        double precision, dimension(:),intent(in) :: vector

        double precision :: logsumexp
        double precision :: maximumlog

        maximumlog = maxval(vector)

        logsumexp =  maximumlog + log(sum(exp(vector - maximumlog)))

    end function logsumexp


    function logaddexp(a,b)
        implicit none
        double precision :: a
        double precision :: b
        double precision :: logaddexp

        if (a>b) then
            logaddexp = a + log(exp(b-a) + 1)
        else
            logaddexp = b + log(exp(a-b) + 1)
        end if

    end function logaddexp

    function logsubexp(a,b)
        implicit none
        double precision :: a
        double precision :: b
        double precision :: logsubexp

        if(a>b) then
            logsubexp = a + log(1-exp(b-a))
        else 
            logsubexp = logzero
        end if

    end function logsubexp





end module utils_module
